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
#include "Buffer.h"
#include "SfzBlockEnvelope.h"
#include <future>

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
    SfzVoice(ThreadPool& fileLoadingPool, SfzFilePool& filePool, const CCValueArray& ccState);
    ~SfzVoice() noexcept;
    
    void startVoiceWithNote(SfzRegion& newRegion, int channel, int noteNumber, uint8_t velocity, int sampleDelay) noexcept;
    void startVoiceWithCC(SfzRegion& newRegion, int channel, int ccNumber, uint8_t ccValue, int sampleDelay) noexcept;
    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void renderNextBlock(AudioBuffer<float>& outputBuffer, int startSample, int numSamples) noexcept;

    void registerAftertouch(int channel, uint8_t aftertouch, int timestamp) noexcept;
    void registerPitchWheel(int channel, int pitch, int timestamp) noexcept;
    void registerNoteOff(int channel, int noteNumber, uint8_t velocity, int timestamp) noexcept;
    void registerCC(int channel, int ccNumber, uint8_t ccValue, int timestamp) noexcept;
    bool checkOffGroup(uint32_t group, int timestamp) noexcept;

    void reset() noexcept;
    bool isFree() const { return state == SfzVoiceState::idle; }
    bool isPlaying() const { return state != SfzVoiceState::idle; }

    std::optional<int> getTriggeringChannel() const noexcept;
    std::optional<int> getTriggeringNoteNumber() const noexcept;
    std::optional<int> getTriggeringCCNumber() const noexcept;
private:
    ThreadPool& fileLoadingPool;
    SfzFilePool& filePool;
    const CCValueArray& ccState;

    // Message and region that activated the note
    std::optional<int> triggeringChannel;
    std::optional<int> triggeringNoteNumber;
    std::optional<int> triggeringCCNumber;
    SfzRegion* region { nullptr };
    std::shared_ptr<AudioBuffer<float>> preloadedData { nullptr };
    std::shared_ptr<AudioBuffer<float>> fileData { nullptr };
    std::atomic<bool> dataReady;

    // Sustain logic
    bool noteIsOff { true };

    // Block configuration
    int samplesPerBlock { config::defaultSamplesPerBlock };
    double sampleRate { config::defaultSampleRate };
    
    // Basic ratios for resampling
    float speedRatio { 1.0 };
    float pitchRatio { 1.0 };
    // Envelopes and states for the voice
    SfzVoiceState state { SfzVoiceState::idle };
    float baseGain { 1.0f };

    SfzEnvelopeGeneratorValue amplitudeEGEnvelope;
    SfzBlockEnvelope<float> amplitudeEnvelope;
    SfzBlockEnvelope<float> panEnvelope;
    SfzBlockEnvelope<float> positionEnvelope;
    SfzBlockEnvelope<float> widthEnvelope;
    HeapBlock<char> tempHeapBlock1;
    HeapBlock<char> tempHeapBlock2;
    dsp::AudioBlock<float> tempBlock1;
    dsp::AudioBlock<float> tempBlock2;
    // Buffer<float> envelopeBuffer { config::defaultSamplesPerBlock };

    // Internal position and counters
    int initialDelay { 0 };
    int sourcePosition { 0 };
    uint32_t loopCount { 1 };

    float decimalPosition { 0.0f };

    JobStatus runJob() override;
    void clearEnvelopes() noexcept;
    void release(int timestamp, bool useFastRelease = false) noexcept;
    void fillBlock(dsp::AudioBlock<float> block) noexcept;
    void fillGenerator(dsp::AudioBlock<float> block) noexcept;
    void fillWithPreloadedData(dsp::AudioBlock<float> block, int releaseOffset) noexcept;
    void fillWithFileData(dsp::AudioBlock<float> block, int releaseOffset) noexcept;
    void commonStartVoice(SfzRegion& newRegion, int sampleDelay) noexcept;
    JUCE_LEAK_DETECTOR(SfzVoice)
};