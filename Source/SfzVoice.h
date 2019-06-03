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

#pragma once
#include "../JuceLibraryCode/JuceHeader.h"
#include "JuceHelpers.h"
#include "SfzRegion.h"
#include "SfzGlobals.h"
#include "SfzEnvelope.h"
#include "SfzCCEnvelope.h"

enum class SfzVoiceState
{
    idle,
    playing,
    release
};

class SfzVoice: public ThreadPoolJob 
{
public:
    SfzVoice() = delete;
    SfzVoice(ThreadPool& fileLoadingPool, SfzFilePool& filePool, const CCValueArray& ccState, int bufferCapacity = config::bufferSize);
    ~SfzVoice();
    
    void startVoice(SfzRegion& newRegion, const MidiMessage& msg, int sampleDelay);
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples);
    void processMidi(MidiMessage& msg, int timestamp);

    bool checkOffGroup(uint32_t group, int timestamp);
    void reset();

    bool isFree() const { return state == SfzVoiceState::idle; }
    bool isPlaying() const { return state != SfzVoiceState::idle; }

    std::optional<MidiMessage> getTriggeringMessage() const;

private:
    void fillBuffer(AudioBuffer<float>& buffer, int startSample, int numSamples);
    ThreadPool& fileLoadingPool;
    SfzFilePool& filePool;
    const CCValueArray& ccState;

    // Fifo/Circular buffer
    mutable AudioBuffer<float> buffer;
    // TODO : No need for shared pointers anymore
    std::shared_ptr<AbstractFifo> fifo;

    // Message and region that activated the note
    MidiMessage triggeringMessage;
    SfzRegion* region { nullptr };
    std::unique_ptr<AudioFormatReader> reader { nullptr };

    // Sustain logic
    bool noteIsOff { true };

    // Block configuration
    int samplesPerBlock { config::defaultSamplesPerBlock };
    double sampleRate { config::defaultSampleRate };
    
    // Basic ratios for resampling
    double speedRatio { 1.0 };
    double pitchRatio { 1.0 };

    // Envelopes and states for the voice
    SfzVoiceState state { SfzVoiceState::idle };
    float baseGain { 1.0 };

    SfzEnvelopeGeneratorValue amplitudeEGEnvelope;
    SfzCCEnvelope amplitudeEnvelope;
    SfzCCEnvelope panEnvelope;
    SfzCCEnvelope positionEnvelope;
    SfzCCEnvelope widthEnvelope;

    // Internal position and counters
    int initialDelay { 0 };
    int64 sourcePosition { 0 };
    uint32_t loopCount { 1 };
    
    uint32_t localTime { 0 };

    // Resamplers ; these need to be in shared pointers because we store the voices in a vector and they're not copy constructible
    // Not very logical since their ownership is not shared, but anyway...
    // TODO : No need for shared pointers anymore
    std::array<std::shared_ptr<LagrangeInterpolator>, config::numChannels> resamplers;

    void release(int timestamp, bool useFastRelease = false);
    void resetResamplers();
    JobStatus runJob() override;

    JUCE_LEAK_DETECTOR(SfzVoice)
};