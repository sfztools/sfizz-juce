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

SfzVoice::SfzVoice(ThreadPool& fileLoadingPool, SfzFilePool& filePool, const CCValueArray& ccState)
: ThreadPoolJob( "SfzVoice" )
, fileLoadingPool(fileLoadingPool)
, filePool(filePool)
, ccState(ccState)
{
}

SfzVoice::~SfzVoice()
{
    if (fileLoadingPool.contains(this))
        fileLoadingPool.removeJob(this, true, 100);
}

void SfzVoice::release(int timestamp, bool useFastRelease)
{
    if (state != SfzVoiceState::release)
    {
        DBG("Sample " << region->sample << " releasing...");
        state = SfzVoiceState::release;
        amplitudeEGEnvelope.release(timestamp, useFastRelease);  
    }
}

void SfzVoice::startVoiceWithNote(SfzRegion& newRegion, int channel, int noteNumber, uint8_t velocity, int sampleDelay)
{
    commonStartVoice(newRegion, sampleDelay);
    triggeringNoteNumber = noteNumber;
    triggeringChannel = channel;
    pitchRatio = region->getBasePitchVariation(noteNumber, velocity);
    baseGain *= region->getNoteGain(noteNumber, velocity);
    amplitudeEGEnvelope.prepare(region->amplitudeEG, ccState, velocity, sampleDelay);
}

void SfzVoice::startVoiceWithCC(SfzRegion& newRegion, int channel, int ccNumber, uint8_t ccValue [[maybe_unused]], int sampleDelay)
{
    commonStartVoice(newRegion, sampleDelay);
    triggeringCCNumber = ccNumber;
    triggeringChannel = channel;
}

void SfzVoice::commonStartVoice(SfzRegion& newRegion, int sampleDelay)
{
    // The voice should be idling!
    jassert(state == SfzVoiceState::idle);
    // Positive delay
    jassert(sampleDelay >= 0);
    if (sampleDelay < 0)
        sampleDelay = 0;

    DBG("Starting voice with " << newRegion.sample);

    auto secondsToSamples = [this](auto timeInSeconds) { 
        return static_cast<int>(timeInSeconds * sampleRate);
    };

    region = &newRegion;
    noteIsOff = false;
    state = SfzVoiceState::playing;

    // Compute the resampling ratio for this region
    speedRatio = static_cast<float>(region->sampleRate / this->sampleRate);

    // Compute the base amplitude gain
    baseGain = region->getBaseGain();

    // Initialize the CC envelopes
    if (region->amplitudeCC)
    {
        amplitudeEnvelope.setFunction([this](uint8_t cc){
            return baseGain * region->amplitudeCC->second * normalizeCC(cc) / 100.0f;}
        );
        amplitudeEnvelope.setDefaultValue(ccState[region->amplitudeCC->first]);
    }
    
    // Initialize the source sample position and add a possibly random offset
    uint32_t totalOffset { region->offset };
    if (region->offsetRandom > 0)
        totalOffset += Random::getSystemRandom().nextInt((int)region->offsetRandom);
    sourcePosition = totalOffset;

    // Now there's possibly an additional sample delay from the region opcodes
    initialDelay = 0;
    if (region->delay > 0)
        initialDelay += secondsToSamples(region->delay);
    if (region->delayRandom > 0)
        initialDelay += Random::getSystemRandom().nextInt(secondsToSamples(region->delayRandom));
    
    preloadedData = filePool.getPreloadedData(region->sample);
    if (preloadedData == nullptr)
        return;

    // Schedule a callback in the background thread
    fileLoadingPool.addJob(this, false);
}

void SfzVoice::registerNoteOff(int channel, int noteNumber, uint8_t velocity [[maybe_unused]], int timestamp)
{
    if (region == nullptr || !triggeringNoteNumber || !triggeringChannel)
        return;
    
    if (state == SfzVoiceState::idle)
        return;

    if (channel != *triggeringChannel)
        return;
    
    if (!noteIsOff && noteNumber == *triggeringNoteNumber && region->loopMode != SfzLoopMode::one_shot)
        noteIsOff = true;
  
    if (noteIsOff && ccState[64] < 64)
        release(timestamp);
}
void SfzVoice::registerAftertouch(int channel, uint8_t aftertouch, int timestamp)
{
    ignoreUnused(channel, aftertouch, timestamp);
}

void SfzVoice::registerPitchWheel(int channel, int pitch, int timestamp)
{
    ignoreUnused(channel, pitch, timestamp);
}

void SfzVoice::registerCC(int channel, int ccNumber, uint8_t ccValue, int timestamp)
{
    if (region == nullptr)
        return;

    if (!withinRange(region->channelRange, channel))
        return;

    if (triggeringCCNumber && *triggeringCCNumber == ccNumber && !withinRange(region->ccTriggers.at(ccNumber), ccValue))
        noteIsOff = true;

    if (noteIsOff && ccState[64] < 64)
        release(timestamp);

    if (region->amplitudeCC && region->amplitudeCC->first == ccNumber)
        amplitudeEnvelope.addEvent(timestamp , ccValue);

    if (region->panCC && region->panCC->first == ccNumber)
        panEnvelope.addEvent(timestamp, ccValue);

    if (region->positionCC && region->positionCC->first == ccNumber)
        positionEnvelope.addEvent(timestamp, ccValue);

    if (region->widthCC && region->widthCC->first == ccNumber)
        widthEnvelope.addEvent(timestamp, ccValue);
}

ThreadPoolJob::JobStatus SfzVoice::runJob()
{
    if (state == SfzVoiceState::idle)
        return ThreadPoolJob::jobHasFinished;

    if (region == nullptr)
        return ThreadPoolJob::jobHasFinished;

    // Normal case: the voice has ended, free up memory and reset the state
    if (state == SfzVoiceState::release)
    {
        reset();
        return ThreadPoolJob::jobHasFinished;
    }
    
    // From here on we load a sample file: generators don't need to do this.
    if (region->isGenerator())
        return ThreadPoolJob::jobHasFinished;

    // TODO: do a switch here?
    // Normal case: we are not releasing so we are playing
    
    const uint32_t endOrLoopEnd = std::min(region->sampleEnd, region->loopRange.getEnd());
    // A >2 gb sample file is a bit unreasonable, but it will cause serious issues
    jassert(endOrLoopEnd <= std::numeric_limits<int>::max());
    const int numSamples = static_cast<int>(endOrLoopEnd);

    
    if (numSamples <= preloadedData->getNumSamples())
    {
        fileData = preloadedData;
    }
    else
    {
        fileData = std::make_shared<AudioBuffer<float>>(config::numChannels, numSamples);
        auto reader = filePool.createReaderFor(region->sample);
        // We should not have a null reader here, something is wrong
        if (reader == nullptr) // still null
        {
            DBG("Could not create reader: something is wrong with the sample " << region->sample);
            return ThreadPoolJob::jobHasFinished;
        }
        reader->read(fileData.get(), 0, numSamples, 0, true, true);
    }

    dataReady = true;
    return ThreadPoolJob::jobHasFinished;
}

void SfzVoice::prepareToPlay(double newSampleRate, int newSamplesPerBlock)
{
    this->sampleRate = newSampleRate;
    this->samplesPerBlock = newSamplesPerBlock;
    amplitudeEGEnvelope.setSampleRate(newSampleRate);
    tempBlock1 = dsp::AudioBlock<float>(tempHeapBlock1, config::numChannels, newSamplesPerBlock);
    tempBlock2 = dsp::AudioBlock<float>(tempHeapBlock2, config::numChannels, newSamplesPerBlock);
    amplitudeEnvelope.reserve(newSamplesPerBlock);
    panEnvelope.reserve(newSamplesPerBlock);
    positionEnvelope.reserve(newSamplesPerBlock);
    widthEnvelope.reserve(newSamplesPerBlock);
    reset();
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

void SfzVoice::fillBlock(dsp::AudioBlock<float> block)
{
    if (region->sample == "*sine")
    {
        const auto frequency = MathConstants<float>::twoPi * MidiMessage::getMidiNoteInHertz(region->pitchKeycenter) * pitchRatio;

        for (int chanIdx = 0; chanIdx < config::numChannels; chanIdx++)
            for(int sampleIdx = 0; sampleIdx < block.getNumSamples(); sampleIdx++)
                block.setSample(chanIdx, sampleIdx, static_cast<float>(std::sin(frequency * localTime++ / sampleRate)));

        return;
    }

    const auto samplesToClear = std::min(initialDelay, (int)block.getNumSamples());
    if (initialDelay > 0)
    {
        block.getSubBlock(0, samplesToClear).clear();
        initialDelay -= samplesToClear;

        if (samplesToClear == block.getNumSamples())
            return;
        else
            block = block.getSubBlock(samplesToClear);
    }

    auto nextPositionBlock = tempBlock1.getSubBlock(0, block.getNumSamples());
    auto interpolationBlock = tempBlock2.getSubBlock(0, block.getNumSamples());

    int nextPosition { 0 };
    if (dataReady)
    {
        const int lastSample { fileData->getNumSamples() - 1 };
        for (auto sampleIdx = 0; sampleIdx < block.getNumSamples(); ++sampleIdx)
        {
            if (sourcePosition > lastSample)
            {
                const int overflow { sourcePosition - lastSample - 1};
                if (region->shouldLoop())
                {
                    sourcePosition = region->loopRange.getStart() + overflow;
                    nextPosition = sourcePosition + 1;
                }
                else if (region->sampleCount && loopCount < *region->sampleCount)
                {
                    // We're looping and counting, restart the source position
                    loopCount += 1;
                    sourcePosition = region->loopRange.getStart() + overflow;
                    nextPosition = sourcePosition + 1;
                }
                else
                {
                    block.getSubBlock(sampleIdx).clear();
                    nextPositionBlock.getSubBlock(sampleIdx).clear();
                    interpolationBlock.getSubBlock(sampleIdx).clear();
                    release(sampleIdx + samplesToClear);
                    break;
                }
            }
            else if (sourcePosition == lastSample)
            {
                if (region->shouldLoop())
                {
                    nextPosition = region->loopRange.getStart();
                }
                else if (region->sampleCount && loopCount < *region->sampleCount)
                {
                    loopCount += 1;
                    nextPosition = region->loopRange.getStart();
                }
                else
                {
                    block.getSubBlock(sampleIdx).clear();
                    nextPositionBlock.getSubBlock(sampleIdx).clear();
                    interpolationBlock.getSubBlock(sampleIdx).clear();
                    release(sampleIdx + samplesToClear);
                    return;
                }
            }
            else
            {
                nextPosition = sourcePosition + 1;
            }

            nextPosition = sourcePosition + 1;
            for (auto chanIdx = 0; chanIdx < config::numChannels; ++chanIdx)
            {
                block.setSample(chanIdx, sampleIdx, fileData->getSample(chanIdx, sourcePosition));
                nextPositionBlock.setSample(chanIdx, sampleIdx, fileData->getSample(chanIdx, nextPosition));
                interpolationBlock.setSample(chanIdx, sampleIdx, decimalPosition);
            }

            decimalPosition += speedRatio * pitchRatio;
            const auto sampleStep = static_cast<int>(decimalPosition);
            sourcePosition += sampleStep;
            decimalPosition -= sampleStep;
        }
    }
    else
    {
        for (auto sampleIdx = 0; sampleIdx < block.getNumSamples(); ++sampleIdx)
        {
            // We need these because the preloaded data may be reused for multiple samples...
            if (    sourcePosition >= preloadedData->getNumSamples() - 1
                ||  sourcePosition >= (region->loopRange.getEnd() - 1) 
                ||  sourcePosition >= (region->sampleEnd - 1))
            {
                block.getSubBlock(sampleIdx).clear();
                nextPositionBlock.getSubBlock(sampleIdx).clear();
                interpolationBlock.getSubBlock(sampleIdx).clear();
                break;
            }

            nextPosition = sourcePosition + 1;
            for (auto chanIdx = 0; chanIdx < config::numChannels; ++chanIdx)
            {
                block.setSample(chanIdx, sampleIdx, preloadedData->getSample(chanIdx, sourcePosition));
                nextPositionBlock.setSample(chanIdx, sampleIdx, preloadedData->getSample(chanIdx, nextPosition));
                interpolationBlock.setSample(chanIdx, sampleIdx, decimalPosition);
            }
            
            decimalPosition += speedRatio * pitchRatio;
            const auto sampleStep = static_cast<int>(decimalPosition);
            sourcePosition += sampleStep;
            decimalPosition -= sampleStep;
        }
    }

    nextPositionBlock.multiply(interpolationBlock);
    interpolationBlock.negate().add(1.0f);
    block.multiply(interpolationBlock).add(nextPositionBlock);
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
        return;
    }

    if (initialDelay > 0)
    {
        const auto samplesToClear = std::min(initialDelay, numSamples);
        outputBuffer.clear(startSample, samplesToClear);
        startSample += samplesToClear;
        numSamples -= samplesToClear;
        initialDelay -= samplesToClear;

        if (numSamples == 0)
            return;
    }

    const auto endSample = startSample + numSamples;
    auto** outputData = outputBuffer.getArrayOfWritePointers();
    auto* leftChannel = outputData[0];
    auto* rightChannel = outputData[1];
    int nextPosition { 0 };
    // Use the preloaded data
    if (!dataReady)
    {
        for (int sampleIdx = startSample; sampleIdx < endSample; sampleIdx++)
        {
            
            // We need these because the preloaded data may be reused for multiple samples...
            if (    sourcePosition >= preloadedData->getNumSamples() - 1
                ||  sourcePosition >= (region->loopRange.getEnd() - 1) 
                ||  sourcePosition >= (region->sampleEnd - 1))
            {
                outputBuffer.clear(startSample, endSample - startSample);
                return;
            }

            nextPosition = sourcePosition + 1;
            const auto complementaryPosition = (1 - decimalPosition);

            leftChannel[sampleIdx] = preloadedData->getSample(0, sourcePosition) * complementaryPosition + preloadedData->getSample(0, nextPosition) * decimalPosition;
            rightChannel[sampleIdx] = preloadedData->getSample(1, sourcePosition) * complementaryPosition + preloadedData->getSample(1, nextPosition) * decimalPosition;

            decimalPosition += speedRatio * pitchRatio;
            const auto sampleStep = static_cast<int>(decimalPosition);
            sourcePosition += sampleStep;
            decimalPosition -= sampleStep;
        }
    }
    else // we have the file data
    {
        const int lastSample { fileData->getNumSamples() - 1 };
        for (int sampleIdx = startSample; sampleIdx < endSample; sampleIdx++)
        {
            if (sourcePosition > lastSample)
            {
                const int overflow { sourcePosition - lastSample - 1};
                if (region->shouldLoop())
                {
                    sourcePosition = region->loopRange.getStart() + overflow;
                    nextPosition = sourcePosition + 1;
                }
                else if (region->sampleCount && loopCount < *region->sampleCount)
                {
                    // We're looping and counting, restart the source position
                    loopCount += 1;
                    sourcePosition = region->loopRange.getStart() + overflow;
                    nextPosition = sourcePosition + 1;
                }
                else
                {
                    release(sampleIdx - startSample);
                    outputBuffer.clear(startSample, endSample - startSample);
                    return;
                }
            }
            else if (sourcePosition == lastSample)
            {
                if (region->shouldLoop())
                {
                    nextPosition = region->loopRange.getStart();
                }
                else if (region->sampleCount && loopCount < *region->sampleCount)
                {
                    loopCount += 1;
                    nextPosition = region->loopRange.getStart();
                }
                else
                {
                    release(sampleIdx - startSample);
                    outputBuffer.clear(startSample, endSample - startSample);
                    return;
                }
            }
            else
            {
                nextPosition = sourcePosition + 1;
            }

            const auto complementaryPosition = (1 - decimalPosition);
            leftChannel[sampleIdx] = fileData->getSample(0, sourcePosition) * complementaryPosition + fileData->getSample(0, nextPosition) * decimalPosition;
            rightChannel[sampleIdx] = fileData->getSample(1, sourcePosition) * complementaryPosition + fileData->getSample(1, nextPosition) * decimalPosition;

            decimalPosition += speedRatio * pitchRatio;
            const auto sampleStep = static_cast<int>(decimalPosition);
            sourcePosition += sampleStep;
            decimalPosition -= sampleStep;
        }
    }
}

void SfzVoice::renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples)
{
    outputBuffer.clear(startSample, numSamples);
    int envelopeIdx = 0;
    
    if (!isPlaying())
        return;

    if (region == nullptr)
        return;
    
    // fillBuffer(outputBuffer, startSample, numSamples);
    auto outputBlock = dsp::AudioBlock<float>(outputBuffer).getSubBlock(startSample, numSamples);
    fillBlock(outputBlock);
    auto localEnvelopeBuffer = tempBlock1.getSubBlock(startSample, numSamples);

    // Amplitude EG envelopes
    for (int sampleIdx = startSample; sampleIdx < numSamples; sampleIdx++)
        outputBuffer.applyGain(sampleIdx, 1, amplitudeEGEnvelope.getNextValue());
    
    if (region->amplitudeCC)
    {
        amplitudeEnvelope.getEnvelope(localEnvelopeBuffer);
        outputBlock.multiply(localEnvelopeBuffer);
    }
    else
    {
        outputBlock.multiply(baseGain);
    }

    if (state == SfzVoiceState::release && !amplitudeEGEnvelope.isSmoothing())
        fileLoadingPool.addJob(this, false);
}

void SfzVoice::reset()
{
    DBG("Reset the voice to its idling state");
    state = SfzVoiceState::idle;
    region = nullptr;
    triggeringNoteNumber.reset();
    triggeringCCNumber.reset();
    triggeringChannel.reset();
    dataReady = false;
    fileData.reset();
    preloadedData.reset();
    initialDelay = 0;
    localTime = 0;
}

std::optional<int> SfzVoice::getTriggeringNoteNumber() const
{
    return triggeringNoteNumber;
}

std::optional<int> SfzVoice::getTriggeringCCNumber() const
{
    return triggeringCCNumber;
}

std::optional<int> SfzVoice::getTriggeringChannel() const
{
    return triggeringChannel;
}