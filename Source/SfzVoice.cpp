/*
    ==============================================================================

    Copyright 2019 - Paul Ferrand (paulfd@outlook.fr)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

    ==============================================================================
*/

#include "SfzVoice.h"

SfzVoice::SfzVoice(ThreadPool& fileLoadingPool, const CCValueArray& ccState, int bufferCapacity)
: ThreadPoolJob( "SfzVoice" )
, fileLoadingPool(fileLoadingPool)
, ccState(ccState)
, fifo(std::make_shared<AbstractFifo>(bufferCapacity))
, buffer(config::numChannels, bufferCapacity)
, tempBuffer(config::numChannels, config::preloadSize)
{
    buffer.clear();
    tempBuffer.clear();
    for (auto& resampler: resamplers)
        resampler = std::make_shared<LagrangeInterpolator>();
    this->samplesPerBlock = samplesPerBlock;
}

SfzVoice::~SfzVoice()
{
}

void SfzVoice::release(int timestamp, bool useFastRelease)
{
    if (state != SfzVoiceState::release)
    {
        region->activeVoices--;
        state = SfzVoiceState::release;
        amplitudeEGEnvelope.release(timestamp, useFastRelease);  
    }
}

void SfzVoice::startVoice(SfzRegion& newRegion, const MidiMessage& msg, int sampleDelay)
{
    // The voice should be idling!
    jassert(state == SfzVoiceState::idle);
    // Positive delay
    jassert(sampleDelay >= 0);
    if (sampleDelay < 0)
        sampleDelay = 0;

    auto secondsToSamples = [this](auto timeInSeconds) { 
        return static_cast<int>(timeInSeconds * sampleRate);
    };

    DBG("Starting voice with delay " << sampleDelay);
    region = &newRegion;
    region->activeVoices++;
    triggeringMessage = msg;
    noteIsOff = false;
    state = SfzVoiceState::playing;

    // Compute the resampling ratio for this region
    speedRatio = region->sampleRate / this->sampleRate;
    pitchRatio = region->computeBasePitchVariation(msg);
    
    // Compute the base amplitude gain
    baseGain = region->computeBaseGain(msg);

    amplitudeEGEnvelope.prepare(region->amplitudeEG, ccState, msg.getVelocity(), sampleDelay);

    // Initialize the CC envelopes
    auto setupLinearCCEnvelope = [this] (SfzCCEnvelope& envelope, const auto& baseValue, const auto& ccSwitch) {
        if (ccSwitch)
        {
            envelope.reset(ccState[ccSwitch->first]);
            float slope = ccSwitch->second;
            envelope.setTransform([this, baseValue, slope] (float ccValue) { return baseValue + slope * ccValue; });
        }
        else
        {
            envelope.reset();
            envelope.setTransform([this, baseValue] (float ccValue) { return baseValue; });
        }
    };

    setupLinearCCEnvelope(amplitudeEnvelope, region->amplitude, region->amplitudeCC);
    setupLinearCCEnvelope(panEnvelope, region->pan, region->panCC);
    if (region->isStereo())
    {
        setupLinearCCEnvelope(positionEnvelope, region->position, region->positionCC);
        setupLinearCCEnvelope(widthEnvelope, region->width, region->widthCC);
    }
    else
    {
        widthEnvelope.reset();
        positionEnvelope.reset();
        widthEnvelope.setTransform([this] (float ccValue) { return 0.0f; });
        positionEnvelope.setTransform([this] (float ccValue) { return 0.0f; });
    }

    // Initialize the source sample position and add a possibly random offset
    uint32_t totalOffset { region->offset };
    if (region->offsetRandom > 0)
        totalOffset += Random::getSystemRandom().nextInt((int)region->offsetRandom);
    sourcePosition = totalOffset;

    // Write a short blank in the circular buffer that corresponds to the noteon delay
    {
        // The buffer should always be cleared at this point so we cheat :)
        AbstractFifo::ScopedWrite writer (*fifo, sampleDelay);
    }

    // Now there's possibly an additional sample delay from the region opcodes
    initialDelay = 0;
    if (region->delay > 0)
        initialDelay += secondsToSamples(region->delay);
    if (region->delayRandom > 0)
        initialDelay += Random::getSystemRandom().nextInt(secondsToSamples(region->delayRandom));;

    auto preloadedData = region->getPreloadedData();
    const auto preloadedDataSize = preloadedData->getNumSamples();

    {
        AbstractFifo::ScopedWrite writer (*fifo, fifo->getFreeSpace());
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }

    {
        const auto samplesToFill = (int)jmin( (uint32_t)fifo->getFreeSpace(), preloadedDataSize - totalOffset);
        AbstractFifo::ScopedWrite writer (*fifo, samplesToFill);
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }

    // Schedule a callback in the background thread
    fileLoadingPool.addJob(this, false);
}

ThreadPoolJob::JobStatus SfzVoice::runJob()
{
    // No need to read anything in idle state
    if (state == SfzVoiceState::idle)
        return ThreadPoolJob::jobHasFinished;

    // Release state ended: we can idle
    if (state == SfzVoiceState::release && !amplitudeEGEnvelope.isSmoothing())
    {
        // Should be valid if not smoothing!!
        // jassert(amplitudeEGEnvelope.getNextValue() <= config::virtuallyZero);
        DBG("Resetting the voice playing " << region->sample);
        reset();
        return ThreadPoolJob::jobHasFinished;
    }

    if (region == nullptr)
    {
        // The region has to be set before reading data
        jassertfalse;
        return ThreadPoolJob::jobHasFinished;
    }

    // We need to delay the source still, so fill in the buffer with zeros some more
    if (initialDelay > 0)
    {
        AbstractFifo::ScopedWrite writer (*fifo, fifo->getFreeSpace());
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }
    
    const auto freeSpace = fifo->getFreeSpace();
    // The buffer is full: nothing to do
    if (freeSpace == 0)
        return ThreadPoolJob::jobHasFinished;

    const auto requiredInputs =  std::min( (int)std::ceil(speedRatio * freeSpace * pitchRatio), tempBuffer.getNumSamples() );
    jassert(sourcePosition <= region->sampleEnd && sourcePosition <= region->loopRange.getEnd());

    auto tempPosition = sourcePosition;
    bool loopingRegion = (region->loopMode == SfzLoopMode::loop_continuous || region->loopMode == SfzLoopMode::loop_sustain);
    // DBG("Reading " << requiredInputs << " samples for source " << region->sample << "(Remaining " << region->sampleEnd - sourcePosition << ")");
    tempBuffer.clear(0, requiredInputs);
    auto filledInputs = 0;

    // Source looping logic
    while (true)
    {
        const auto samplesLeft = region->sampleEnd - tempPosition;
        const auto samplesLeftInLoop = region->loopRange.getEnd() - tempPosition;
        const auto numSamplesToRead = (int)jmin(samplesLeft, samplesLeftInLoop, (int64)(requiredInputs - filledInputs));        
        filledInputs += readInputs;
        tempPosition += readInputs;

        // We reached the sample end or sample loop end
        if (tempPosition == region->sampleEnd || tempPosition == region->loopRange.getEnd() )
        {
            if (loopingRegion)
            {
                // DBG("Sample " << region->sample << " end! Looping...");
                // We're looping, restart the source position
                // TODO: make a function and crossfade before AND after...
                jassert(region->loopRange.getStart() > config::loopCrossfadeLength);
                const auto localXFLength = jmin(filledInputs, config::loopCrossfadeLength);
                tempBuffer.applyGainRamp(filledInputs - localXFLength, localXFLength, 1, 0);
                region->readFromSource(crossfadeBuffer, 0, localXFLength, region->loopRange.getStart() - localXFLength);
                crossfadeBuffer.applyGainRamp(0, localXFLength, 0, 1);
                for (int chanIdx = 0; chanIdx < config::numChannels; chanIdx++)
                    tempBuffer.addFrom(chanIdx, filledInputs - localXFLength, crossfadeBuffer, chanIdx, 0, localXFLength);
                tempPosition = region->loopRange.getStart();
            }
            else if (region->sampleCount && loopCount < *region->sampleCount)
            {
                // DBG("Sample " << region->sample << " end! Looping (count)...");
                // We're looping and counting, restart the source position
                loopCount += 1;
                tempPosition = region->loopRange.getStart();
            }
            else
            {
                DBG("Sample " << region->sample << " end! Releasing...");
                // we reached the end of the sample, move on to a release state
                // TODO: this is slightly wrong..
                release(static_cast<int>(filledInputs / requiredInputs * freeSpace) + fifo->getNumReady());
                break;
            }
        }
        
        if (filledInputs == requiredInputs)
            break;

        if (shouldExit())
            return ThreadPoolJob::jobNeedsRunningAgain;
    }
    
    // Resample
    int consumedSamples { 0 };
    {
        AbstractFifo::ScopedWrite writer (*fifo, filledInputs);
        consumedSamples += resampleStream(tempBuffer, 0, filledInputs, buffer, writer.startIndex1, writer.blockSize1);
        consumedSamples += resampleStream(tempBuffer, consumedSamples, filledInputs - consumedSamples, buffer, writer.startIndex2, writer.blockSize2);
    }

    // We may need to back up a bit if we did not consume everything
    // TODO: add the proper looping logic, not sure this one works well with crossfading and such
    bool stillNeedsToLoop = (loopingRegion || (region->sampleCount && loopCount <= *region->sampleCount));
    if (stillNeedsToLoop && withinRange(region->loopRange, tempPosition))
    {
        sourcePosition = loopInRange(region->loopRange, sourcePosition + consumedSamples);
    }
    else
    {
        sourcePosition += consumedSamples;
    }

    // We should not go beyond the loop end or the sample end...
    jassert(sourcePosition <= region->sampleEnd && sourcePosition <= region->loopRange.getEnd());

    return ThreadPoolJob::jobHasFinished;
}

void SfzVoice::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    this->sampleRate = sampleRate;
    amplitudeEGEnvelope.setSampleRate(sampleRate);
    reset();
}

void SfzVoice::processMidi(MidiMessage& msg, int timestamp)
{
    if (region == nullptr)
        return;

    if (state == SfzVoiceState::idle)
        return;
    
    if (msg.isNoteOff() 
        && !noteIsOff
        && msg.getNoteNumber() == triggeringMessage.getNoteNumber()
        && region->loopMode != SfzLoopMode::one_shot)
    {
        noteIsOff = true;
    }

    if (msg.isController())
    {
        auto ccIdx = msg.getControllerNumber();
        auto ccValue = msg.getControllerValue();

        if (triggeringMessage.isController()
            && triggeringMessage.getControllerNumber() == ccIdx
            && !withinRange(region->ccTriggers.get(ccIdx), ccValue))
        {
            noteIsOff = true;
        }

        if (region->amplitudeCC && region->amplitudeCC->first == ccIdx)
            amplitudeEnvelope.addEvent(timestamp, ccValue);

        if (region->panCC && region->panCC->first == ccIdx)
            panEnvelope.addEvent(timestamp, ccValue);

        if (region->positionCC && region->positionCC->first == ccIdx)
            positionEnvelope.addEvent(timestamp, ccValue);

        if (region->widthCC && region->widthCC->first == ccIdx)
            widthEnvelope.addEvent(timestamp, ccValue);
        
        // if (region->volumeCCTriggers.contains(ccIdx))
        //     addEventInFifo(amplitudeCC, amplitudeCCFifo, timestamp, region->volumeCCTriggers[ccIdx] * normalizeCC(ccValue));
    }

    if (noteIsOff && ccState[64] < 64)
    {
        release(timestamp);
    }
}

bool SfzVoice::checkOffGroup(uint32_t group, int timestamp)
{
    if (region != nullptr && region->offBy && *region->offBy == group)
    {
        release(timestamp, region->offMode == SfzOffMode::fast);
        return true;
    }

    return false;
}

void SfzVoice::fillBuffer(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    if (region->sample == "*sine")
    {
        const auto frequency = MathConstants<float>::twoPi * MidiMessage::getMidiNoteInHertz(region->pitchKeycenter) * pitchRatio;
        auto time = localTime;
        const auto endSample = startSample + numSamples;

        for(int sampleIdx = startSample; sampleIdx < endSample; sampleIdx++)
        {
            const float sampleValue = static_cast<float>(std::sin(frequency * time / sampleRate));
            for (int chanIdx = 0; chanIdx < config::numChannels; chanIdx++)
                buffer.setSample(chanIdx, sampleIdx, sampleValue);
            
            time++;
        }
    }
    else
    {
        int start1, size1, start2, size2;
        fifo->prepareToRead(fifo->getNumReady(), start1, size1, start2, size2);
        int fifoIdx { start1 };
        int availableSamples { size1 };
        int wrapAround { start1 + size1 - start2 };
        int endPoint = start2 + size2;
        int consumedSamples { 0 };

        // Sample reading and pitch correction
        for (int sampleIdx = startSample; sampleIdx < numSamples; sampleIdx++)
        {
            auto* leftIn = buffer.getReadPointer(0, fifoIdx);
            auto* rightIn = buffer.getReadPointer(1, fifoIdx);
            auto* leftOut = outputBuffer.getWritePointer(0, sampleIdx);
            auto* rightOut = outputBuffer.getWritePointer(1, sampleIdx);
            resamplers[0]->process(pitchRatio * speedRatio, leftIn, leftOut, 1, availableSamples, wrapAround);
            int samplesRead = resamplers[1]->process(pitchRatio * speedRatio, rightIn, rightOut, 1, availableSamples, wrapAround);
            
            fifoIdx += samplesRead;
            availableSamples -= samplesRead;
            consumedSamples += samplesRead;
            if (availableSamples == 0)
            {
                if (fifoIdx == endPoint || size2 == 0)
                {
                    break;
                }
                else
                {
                    fifoIdx = start2;
                    availableSamples = size2;
                    wrapAround = 0;
                }
            }
        }

        fifo->finishedRead(consumedSamples);
    }
}

void SfzVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    outputBuffer.clear(startSample, numSamples);
    
    if (!isPlaying())
        return;

    if (region == nullptr)
        return;
    
    fillBuffer(outputBuffer, startSample, numSamples);

    // Amplitude envelopes
    for (int sampleIdx = startSample; sampleIdx < numSamples; sampleIdx++)
    {
        float totalGain { baseGain };
        float& left { *outputBuffer.getWritePointer(0, sampleIdx) };
        float& right { *outputBuffer.getWritePointer(1, sampleIdx) };
        applyPanToSample(panEnvelope.getNextValue(), left, right);
        applyWidthAndPositionToSample(widthEnvelope.getNextValue(), positionEnvelope.getNextValue(), left, right);

        // Apply a total gain
        totalGain *= amplitudeEGEnvelope.getNextValue();
        totalGain *= amplitudeEnvelope.getNextValue() / SfzDefault::amplitudeRange.getEnd();
        left *= totalGain;
        right *= totalGain;

        // Time advances!
        localTime++;
    }

    // applyAmplitudeEnvelope(outputBuffer, startSample, numSamples);
    // incrementTime(numSamples);
    fileLoadingPool.addJob(this, false);
}

void SfzVoice::reset()
{
    state = SfzVoiceState::idle;
    region = nullptr;
    initialDelay = 0;
    localTime = 0;
    resetResamplers();
    buffer.clear();
    tempBuffer.clear();
    fifo->reset();
}

std::optional<MidiMessage> SfzVoice::getTriggeringMessage() const
{
    if (isPlaying())
        return triggeringMessage;
    else
        return {};
}

int SfzVoice::resampleStream(const AudioBuffer<float>& bufferToResample, int startSampleInput, int numSamplesInput, AudioBuffer<float>& outputBuffer, int startSampleOutput, int numSamplesOutput)
{
    int consumedSamples { 0 };
    for (int channelIdx = 0; channelIdx < config::numChannels; ++channelIdx)
    {
        auto* dataIn = bufferToResample.getReadPointer(channelIdx) + startSampleInput;
        auto* dataOut = outputBuffer.getWritePointer(channelIdx) + startSampleOutput;
        consumedSamples = resamplers[channelIdx]->process(speedRatio * pitchRatio, dataIn, dataOut, numSamplesOutput, numSamplesInput, 0);
    }
    return consumedSamples;
}

int SfzVoice::readFromSource(AudioBuffer<float>& buffer, int startSample, int numSamples, uint32_t positionInFile, bool canBlock)
{
    if (region == nullptr)
        return 0;

    buffer.clear(startSample, numSamples);

    if (region->sample.startsWithChar('*'))
    {
        if (region->sample == "*silence")
            return numSamples;
        
        if (region->sample == "*sine")
        {
            const auto frequency = MathConstants<float>::twoPi * MidiMessage::getMidiNoteInHertz(region->pitchKeycenter) * pitchRatio;
            const auto endSample = startSample + numSamples;

            for(int sampleIdx = startSample; sampleIdx < endSample; sampleIdx++)
            {
                const float sampleValue = static_cast<float>(std::sin(frequency * positionInFile / sampleRate));
                for (int chanIdx = 0; chanIdx < config::numChannels; chanIdx++)
                    buffer.setSample(chanIdx, sampleIdx, sampleValue);
                
                positionInFile++;
            }

            return numSamples;
        }
    }
    
    if (preloadedData == nullptr && !canBlock)
        return 0;
    
    
}

void SfzVoice::resetResamplers()
{
    for (auto& resampler: resamplers)
        resampler->reset();
}