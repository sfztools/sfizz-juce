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
#include "SfzGlobals.h"
#include "SfzDefaults.h"
#include "SfzContainer.h"
#include "SfzOpcode.h"
#include "SfzEnvelope.h"
#include "SfzFilePool.h"
#include "JuceHelpers.h"
#include <string>
#include <optional>
#include <array>
#include <map>

struct SfzRegion
{
    SfzRegion() = delete;
    SfzRegion(const File& root, SfzFilePool& filePool);
    void parseOpcode(const SfzOpcode& opcode);
    String stringDescription() const;
    bool checkMidiConditions(const MidiMessage& msg) const;
    void updateSwitches(const MidiMessage& msg);
    bool appliesTo(const MidiMessage& msg, float randValue) const;
    bool prepare();
    bool isStereo() const;
    float velocityGaindB(int8 velocity) const;
    double computeBasePitchVariation(const MidiMessage& msg) const
    {
        auto pitchVariationInCents = pitchKeytrack * (msg.getNoteNumber() - (int)pitchKeycenter); // note difference with pitch center
        pitchVariationInCents += tune; // sample tuning
        pitchVariationInCents += config::centPerSemitone * transpose; // sample transpose
        pitchVariationInCents += static_cast<int>(msg.getFloatVelocity() * pitchVeltrack); // track velocity
        if (pitchRandom > 0)
            pitchVariationInCents += Random::getSystemRandom().nextInt((int)pitchRandom * 2) - pitchRandom; // random pitch changes
        return centsFactor(pitchVariationInCents);
    }
    float computeBaseGain(const MidiMessage& msg) const
    {
        float baseGaindB { volume };
        baseGaindB += (2 * Random::getSystemRandom().nextFloat() - 1) * ampRandom;
        if (trigger != SfzTrigger::release_key)
            baseGaindB += velocityGaindB(msg.getVelocity());
        else
            baseGaindB += velocityGaindB(lastNoteVelocities[msg.getNoteNumber()]);
        return Decibels::decibelsToGain(baseGaindB);
    }
    bool isRelease() const { return trigger == SfzTrigger::release || trigger == SfzTrigger::release_key; }
    bool isSwitchedOn() const;
    bool isGenerator() { return sample.startsWithChar('*'); }

    // Sound source: sample playback
    String sample {}; // Sample
    float delay { SfzDefault::delay }; // delay
    float delayRandom { SfzDefault::delayRandom }; // delay_random
    uint32_t offset { SfzDefault::offset }; // offset
    uint32_t offsetRandom { SfzDefault::offsetRandom }; // offset_random
    uint32_t sampleEnd { SfzDefault::sampleEndRange.getEnd() }; // end
    std::optional<uint32_t> sampleCount {}; // count
    SfzLoopMode loopMode { SfzDefault::loopMode }; // loopmode
    Range<uint32_t> loopRange { SfzDefault::loopRange }; //loopstart and loopend

    // Instrument settings: voice lifecycle
    uint32_t group { SfzDefault::group }; // group
    std::optional<uint32_t> offBy {}; // off_by
    SfzOffMode offMode { SfzDefault::offMode }; // off_mode

    // Region logic: key mapping
    Range<uint8_t> keyRange{ SfzDefault::keyRange }; //lokey, hikey and key
    Range<uint8_t> velocityRange{ SfzDefault::velocityRange }; // hivel and lovel

    // Region logic: MIDI conditions
    Range<uint8_t> channelRange{ SfzDefault::channelRange }; //lochan and hichan
    Range<int> bendRange{ SfzDefault::bendRange }; // hibend and lobend
    SfzContainer<Range<uint8_t>> ccConditions { SfzDefault::ccRange };
    Range<uint8_t> keyswitchRange{ SfzDefault::keyRange }; // sw_hikey and sw_lokey
    std::optional<uint8_t> keyswitch {}; // sw_last
    std::optional<uint8_t> keyswitchUp {}; // sw_up
    std::optional<uint8_t> keyswitchDown {}; // sw_down
    std::optional<uint8_t> previousNote {}; // sw_previous
    SfzVelocityOverride velocityOverride { SfzDefault::velocityOverride }; // sw_vel

    // Region logic: internal conditions
    Range<uint8_t> aftertouchRange{ SfzDefault::aftertouchRange }; // hichanaft and lochanaft
    Range<float> bpmRange{ SfzDefault::bpmRange }; // hibpm and lobpm
    Range<float> randRange{ SfzDefault::randRange }; // hirand and lorand
    uint8_t sequenceLength { SfzDefault::sequenceLength }; // seq_length
    uint8_t sequencePosition { SfzDefault::sequencePosition }; // seq_position

    // Region logic: triggers
    SfzTrigger trigger { SfzDefault::trigger }; // trigger
    std::array<uint8_t, 128> lastNoteVelocities; // Keeps the velocities of the previous note-ons if the region has the trigger release_key
    SfzContainer<Range<uint8_t>> ccTriggers { SfzDefault::ccTriggerValueRange }; // on_loccN on_hiccN

    // Performance parameters: amplifier
    float volume { SfzDefault::volume }; // volume
    float amplitude { SfzDefault::amplitude }; // amplitude
    float pan { SfzDefault::pan }; // pan
    float width { SfzDefault::width }; // width
    float position { SfzDefault::position }; // position
    std::optional<CCValuePair> volumeCC; // volume_oncc
    std::optional<CCValuePair> amplitudeCC; // amplitude_oncc
    std::optional<CCValuePair> panCC; // pan_oncc
    std::optional<CCValuePair> widthCC; // width_oncc
    std::optional<CCValuePair> positionCC; // position_oncc
    uint8_t ampKeycenter { SfzDefault::ampKeycenter }; // amp_keycenter
    float ampKeytrack { SfzDefault::ampKeytrack }; // amp_keytrack
    float ampVeltrack { SfzDefault::ampVeltrack }; // amp_keytrack
    std::vector<std::pair<int, float>> velocityPoints; // amp_velcurve_N
    float ampRandom { SfzDefault::ampRandom }; // amp_random

    // Performance parameters: pitch
    uint8_t pitchKeycenter{ SfzDefault::pitchKeycenter }; // pitch_keycenter
    int pitchKeytrack{ SfzDefault::pitchKeytrack }; // pitch_keytrack
    int pitchRandom{ SfzDefault::pitchRandom }; // pitch_random
    int pitchVeltrack{ SfzDefault::pitchVeltrack }; // pitch_veltrack
    int transpose { SfzDefault::transpose }; // transpose
    int tune { SfzDefault::tune }; // tune

    // Envelopes
    SfzEnvelopeGeneratorDescription amplitudeEG;
    SfzEnvelopeGeneratorDescription pitchEG;
    SfzEnvelopeGeneratorDescription filterEG;

    double sampleRate { config::defaultSampleRate };
    int numChannels { 1 };
    // TODO : Transform regions to list and put this as atomic
    int activeVoices { 0 }; 
    std::atomic<int> activeNotesInRange { -1 };

    std::vector<SfzOpcode> unknownOpcodes;
    std::shared_ptr<AudioBuffer<float>> preloadedData;
private:
    bool prepared { false };
    File rootDirectory { File::getCurrentWorkingDirectory() };
    
    // File information
    SfzFilePool& filePool;

    // Activation logics
    bool keySwitched { true };
    bool previousKeySwitched { true };
    bool sequenceSwitched { true };
    std::array<bool, 128> ccSwitched;
    bool pitchSwitched { true };
    bool bpmSwitched { true };
    bool aftertouchSwitched { true };

    int sequenceCounter { 0 };
    bool setupSource();
    void addEndpointsToVelocityCurve();
    void checkInitialConditions();
    JUCE_LEAK_DETECTOR (SfzRegion)
};