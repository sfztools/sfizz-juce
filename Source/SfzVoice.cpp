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
, fifo(bufferCapacity)
, buffer(config::numChannels, bufferCapacity)
{
    buffer.clear();
}

SfzVoice::~SfzVoice()
{
}

void SfzVoice::release(int timestamp, bool useFastRelease)
{
    if (state != SfzVoiceState::release)
    {
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

void SfzVoice::startVoiceWithCC(SfzRegion& newRegion, int channel, int ccNumber, uint8_t ccValue, int sampleDelay)
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

    auto secondsToSamples = [this](auto timeInSeconds) { 
        return static_cast<int>(timeInSeconds * sampleRate);
    };

    region = &newRegion;
    noteIsOff = false;
    state = SfzVoiceState::playing;

    // Compute the resampling ratio for this region
    speedRatio = region->sampleRate / this->sampleRate;

    // Compute the base amplitude gain
    baseGain = region->getBaseGain();

    // Initialize the CC envelopes
    if (region->amplitudeCC)
    {
        amplitudeEnvelope.setFunction([this](uint8_t cc){
            return baseGain + region->amplitudeCC->second * normalizeCC(cc) / 100.0f;}
        );
        amplitudeEnvelope.setDefaultValue(ccState[region->amplitudeCC->first]);
    }
    
    // Initialize the source sample position and add a possibly random offset
    uint32_t totalOffset { region->offset };
    if (region->offsetRandom > 0)
        totalOffset += Random::getSystemRandom().nextInt((int)region->offsetRandom);
        
    sourcePosition = totalOffset;

    // Write a short blank in the circular buffer that corresponds to the noteon delay
    {
        // The buffer should always be cleared at this point so we cheat :)
        AbstractFifo::ScopedWrite writer (fifo, sampleDelay);
    }

    // Now there's possibly an additional sample delay from the region opcodes
    initialDelay = 0;
    if (region->delay > 0)
        initialDelay += secondsToSamples(region->delay);
    if (region->delayRandom > 0)
        initialDelay += Random::getSystemRandom().nextInt(secondsToSamples(region->delayRandom));
    if (initialDelay < fifo.getFreeSpace())
    {
        AbstractFifo::ScopedWrite writer (fifo, initialDelay);
        initialDelay = 0;
    }
    else
    {
        AbstractFifo::ScopedWrite writer (fifo, fifo.getFreeSpace());
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }

    preloadedData = filePool.getPreloadedData(region->sample);
    if (preloadedData == nullptr)
        return;

    const auto preloadedDataSize = preloadedData->getNumSamples();
    const auto samplesToFill = (int)std::min( (uint32_t)fifo.getFreeSpace(), preloadedDataSize - totalOffset);

    {
        AbstractFifo::ScopedWrite writer (fifo, samplesToFill);
        copyBuffers(*preloadedData, totalOffset, buffer, writer.startIndex1, writer.blockSize1);
        copyBuffers(*preloadedData, totalOffset + writer.blockSize1, buffer, writer.startIndex2, writer.blockSize2);
    }

    sourcePosition = totalOffset + samplesToFill;

    // Schedule a callback in the background thread
    fileLoadingPool.addJob(this, false);
}

void SfzVoice::registerNoteOff(int channel, int noteNumber, uint8_t velocity, int timestamp)
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
    // No need to read anything in idle state
    if (state == SfzVoiceState::idle)
        return ThreadPoolJob::jobHasFinished;

    // Release state ended: we can idle
    if (state == SfzVoiceState::release && !amplitudeEGEnvelope.isSmoothing())
    {
        // Should be valid if not smoothing!!
        DBG("Resetting the voice playing " << region->sample);
        reset();
        return ThreadPoolJob::jobHasFinished;
    }

    // The region has to be set before reading data
    if (region == nullptr)
        return ThreadPoolJob::jobHasFinished;

    // Nothing to preload here
    if (region->isGenerator())
        return ThreadPoolJob::jobHasFinished;

    // We should have data in there, so something is wrong
    if (preloadedData == nullptr)
        return ThreadPoolJob::jobHasFinished;

    // We need to delay the source still, so fill in the buffer with zeros some more
    if (initialDelay > 0)
    {
        AbstractFifo::ScopedWrite writer (fifo, fifo.getFreeSpace());
        initialDelay -= writer.blockSize1 + writer.blockSize2;
    }
    
    // The buffer is full: nothing to do
    if (fifo.getFreeSpace() == 0)
        return ThreadPoolJob::jobHasFinished;

    jassert(sourcePosition <= region->sampleEnd && sourcePosition <= region->loopRange.getEnd());
    bool loopingSample = (region->loopMode == SfzLoopMode::loop_continuous || region->loopMode == SfzLoopMode::loop_sustain);
    // DBG("Reading " << requiredInputs << " samples for source " << region->sample << "(Remaining " << region->sampleEnd - sourcePosition << ")");

    // Source looping logic
    const auto endOrLoopEnd = std::min(region->sampleEnd, region->loopRange.getEnd());
    while (true)
    {
        const auto samplesLeft = endOrLoopEnd - sourcePosition;
        const auto numSamplesToRead = (int)std::min(samplesLeft, (int64)fifo.getFreeSpace());
        const auto preloadedSamplesLeft = (int)((int64)preloadedData->getNumSamples() - sourcePosition);

        if (preloadedSamplesLeft > 0)
        {
            const auto numSamplesToReadPreloaded = std::min(numSamplesToRead, preloadedSamplesLeft);
            AbstractFifo::ScopedWrite writer (fifo, numSamplesToReadPreloaded);
            copyBuffers(*preloadedData, sourcePosition, buffer, writer.startIndex1, writer.blockSize1);
            sourcePosition += writer.blockSize1;
            copyBuffers(*preloadedData, sourcePosition, buffer, writer.startIndex2, writer.blockSize2);
            sourcePosition += writer.blockSize2;
        }
        else if (numSamplesToRead > 0)
        {
            if (reader == nullptr)
            {
                reader = filePool.createReaderFor(region->sample);
                if (reader == nullptr) // still null
                {
                    DBG("Something is wrong with the sample " << region->sample);
                    return ThreadPoolJob::jobHasFinished;
                }
            }
            AbstractFifo::ScopedWrite writer (fifo, numSamplesToRead);
            reader->read(&buffer, writer.startIndex1, writer.blockSize1, sourcePosition, true, true);
            sourcePosition += writer.blockSize1;
            reader->read(&buffer, writer.startIndex2, writer.blockSize2, sourcePosition, true, true);
            sourcePosition += writer.blockSize2;
        }

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
                release(fifo.getNumReady());
                break;
            }
        }

        if (fifo.getFreeSpace() == 0)
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
    envelopeBuffer.resize(samplesPerBlock);
    amplitudeEnvelope.reserve(samplesPerBlock);
    panEnvelope.reserve(samplesPerBlock);
    positionEnvelope.reserve(samplesPerBlock);
    widthEnvelope.reserve(samplesPerBlock);
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
        const int availableSamples = fifo.getNumReady();
        fifo.prepareToRead(availableSamples, start1, size1, start2, size2);
        int fifoIdx { start1 };
        int end1 { start1 + size1 - 1};
        int nextIdx { 0 };
        int consumedSamples { 0 };

        auto** outputData = outputBuffer.getArrayOfWritePointers();
        auto* leftChannel = outputData[0];
        auto* rightChannel = outputData[1];
        // Sample reading and pitch correction
        for (int sampleIdx = startSample; sampleIdx < numSamples; sampleIdx++)
        {
            auto* leftOut = outputBuffer.getWritePointer(0, sampleIdx);
            auto* rightOut = outputBuffer.getWritePointer(1, sampleIdx);

            if (fifoIdx == end1)
            {
                nextIdx = start2;
            }
            else if (fifoIdx > end1)
            {
                fifoIdx = start2 + fifoIdx - end1 - 1;
                nextIdx = fifoIdx + 1;
            }
            else
            {
                nextIdx = fifoIdx + 1;
            }
            
            const auto complementaryPosition = (1 - decimalPosition);
            leftChannel[sampleIdx] = buffer.getSample(0, fifoIdx) * complementaryPosition + buffer.getSample(0, nextIdx) * decimalPosition;
            rightChannel[sampleIdx] = buffer.getSample(1, fifoIdx) * complementaryPosition + buffer.getSample(1, nextIdx) * decimalPosition;
            
            decimalPosition += speedRatio * pitchRatio;
            const auto sampleStep = static_cast<int>(decimalPosition);

            fifoIdx += sampleStep;
            decimalPosition -= sampleStep;
            consumedSamples += sampleStep;

            if (consumedSamples > availableSamples)
                break;
        }

        fifo.finishedRead(consumedSamples);
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
    
    fillBuffer(outputBuffer, startSample, numSamples);

    // Amplitude EG envelopes
    for (int sampleIdx = startSample; sampleIdx < numSamples; sampleIdx++)
        outputBuffer.applyGain(sampleIdx, 1, amplitudeEGEnvelope.getNextValue());
    
    if (region->amplitudeCC)
    {
        amplitudeEnvelope.getEnvelope(envelopeBuffer.data(), numSamples);
        for (int sampleIdx = startSample; sampleIdx < numSamples; sampleIdx++)
            outputBuffer.applyGain(sampleIdx, 1, envelopeBuffer[envelopeIdx++]);
    }
    else
    {
        outputBuffer.applyGain(baseGain);
    }

    if (!fileLoadingPool.contains(this))
        fileLoadingPool.addJob(this, false);
}

void SfzVoice::reset()
{
    state = SfzVoiceState::idle;
    region = nullptr;
    triggeringNoteNumber.reset();
    triggeringCCNumber.reset();
    triggeringChannel.reset();
    reader.reset();
    preloadedData.reset();
    initialDelay = 0;
    localTime = 0;
    buffer.clear();
    fifo.reset();
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