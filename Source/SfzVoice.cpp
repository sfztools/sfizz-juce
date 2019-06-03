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

SfzVoice::SfzVoice(ThreadPool& fileLoadingPool, SfzFilePool& filePool, const CCValueArray& ccState, int bufferCapacity)
: ThreadPoolJob( "SfzVoice" )
, fileLoadingPool(fileLoadingPool)
, filePool(filePool)
, ccState(ccState)
, fifo(std::make_shared<AbstractFifo>(bufferCapacity))
, buffer(config::numChannels, bufferCapacity)
{
    buffer.clear();
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
        initialDelay += Random::getSystemRandom().nextInt(secondsToSamples(region->delayRandom));
    if (initialDelay < fifo->getFreeSpace())
    {
        AbstractFifo::ScopedWrite writer (*fifo, initialDelay);
        initialDelay = 0;
    }
    else
    {
        AbstractFifo::ScopedWrite writer (*fifo, fifo->getFreeSpace());
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }

    auto preloadedData = filePool.getPreloadedData(region->sample);
    if (preloadedData == nullptr)
        return;

    const auto preloadedDataSize = preloadedData->getNumSamples();
    const auto samplesToFill = (int)std::min( (uint32_t)fifo->getFreeSpace(), preloadedDataSize - totalOffset);

    {
        AbstractFifo::ScopedWrite writer (*fifo, samplesToFill);
        copyBuffers(*preloadedData, totalOffset, buffer, writer.startIndex1, writer.blockSize1);
        copyBuffers(*preloadedData, totalOffset + writer.blockSize1, buffer, writer.startIndex2, writer.blockSize2);
    }

    sourcePosition = totalOffset + samplesToFill;

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

    if (region->isGenerator())
    {
        // Nothing to preload here
        return ThreadPoolJob::jobHasFinished;
    }

    if (reader == nullptr)
    {
        reader = filePool.createReaderFor(region->sample);
        if (reader == nullptr) // still null
        {
            DBG("Something is wrong with the sample " << region->sample);
            return ThreadPoolJob::jobHasFinished;
        }
    }

    // We need to delay the source still, so fill in the buffer with zeros some more
    if (initialDelay > 0)
    {
        AbstractFifo::ScopedWrite writer (*fifo, fifo->getFreeSpace());
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }
    
    auto freeSpace = fifo->getFreeSpace();
    // The buffer is full: nothing to do
    if (freeSpace == 0)
        return ThreadPoolJob::jobHasFinished;

    jassert(sourcePosition <= region->sampleEnd && sourcePosition <= region->loopRange.getEnd());
    bool loopingSample = (region->loopMode == SfzLoopMode::loop_continuous || region->loopMode == SfzLoopMode::loop_sustain);
    // DBG("Reading " << requiredInputs << " samples for source " << region->sample << "(Remaining " << region->sampleEnd - sourcePosition << ")");

    // Source looping logic
    while (true)
    {
        const auto samplesLeft = region->sampleEnd - sourcePosition;
        const auto samplesLeftInLoop = region->loopRange.getEnd() - sourcePosition;
        const auto numSamplesToRead = (int)jmin(samplesLeft, samplesLeftInLoop, (int64)freeSpace);
        {
            AbstractFifo::ScopedWrite writer (*fifo, numSamplesToRead);
            reader->read(&buffer, writer.startIndex1, writer.blockSize1, sourcePosition, true, true);
            sourcePosition += writer.blockSize1;
            reader->read(&buffer, writer.startIndex2, writer.blockSize2, sourcePosition, true, true);
            sourcePosition += writer.blockSize2;
        }
        freeSpace -= numSamplesToRead;

        // We reached the sample end or sample loop end
        if (sourcePosition == region->sampleEnd || sourcePosition == region->loopRange.getEnd() )
        {
            if (loopingSample)
            {
                sourcePosition = region->loopRange.getStart();
            }
            else if (region->sampleCount && loopCount < *region->sampleCount)
            {
                // We're looping and counting, restart the source position
                loopCount += 1;
                sourcePosition = region->loopRange.getStart();
            }
            else
            {
                DBG("Sample " << region->sample << " end! Releasing...");
                release(fifo->getNumReady());
                break;
            }
        }
        
        if (freeSpace == 0)
            break;

        if (shouldExit())
            return ThreadPoolJob::jobNeedsRunningAgain;
    }
    
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
                outputBuffer.setSample(chanIdx, sampleIdx, sampleValue);
            
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
    if (!fileLoadingPool.contains(this))
        fileLoadingPool.addJob(this, false);
}

void SfzVoice::reset()
{
    state = SfzVoiceState::idle;
    region = nullptr;
    reader.reset();
    initialDelay = 0;
    localTime = 0;
    resetResamplers();
    buffer.clear();
    fifo->reset();
}

std::optional<MidiMessage> SfzVoice::getTriggeringMessage() const
{
    if (isPlaying())
        return triggeringMessage;
    else
        return {};
}

void SfzVoice::resetResamplers()
{
    for (auto& resampler: resamplers)
        resampler->reset();
}