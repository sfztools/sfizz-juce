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

#include "SfzRegion.h"

SfzRegion::SfzRegion(const File& root, SfzFilePool& filePool)
: rootDirectory(root), filePool(filePool)
{
    for (auto& ccSwitch: ccSwitched)
        ccSwitch = true;
}

void SfzRegion::parseOpcode(const SfzOpcode& opcode)
{
    prepared = false; // region state changed
    switch (hash(opcode.opcode))
    {
    // Sound source: sample playback
    case hash("sample"): 
        sample = String(opcode.value.data(), opcode.value.length()).trim().replaceCharacter('\\', '/');
    break;
    case hash("delay"): setValueFromOpcode(opcode, delay, SfzDefault::delayRange); break;
    case hash("delay_random"): setValueFromOpcode(opcode, delayRandom, SfzDefault::delayRange); break;
    case hash("offset"): setValueFromOpcode(opcode, offset, SfzDefault::offsetRange); break;
    case hash("offset_random"): setValueFromOpcode(opcode, offsetRandom, SfzDefault::offsetRange); break;
    case hash("end"): setValueFromOpcode(opcode, sampleEnd, SfzDefault::sampleEndRange); break;
    case hash("count"): setValueFromOpcode(opcode, sampleCount, SfzDefault::sampleCountRange); break;
    case hash("loopmode"):
    case hash("loop_mode"):
        switch(hash(opcode.value))
        {
            case hash("no_loop"):
                loopMode = SfzLoopMode::no_loop;
                break;
            case hash("one_shot"):
                loopMode = SfzLoopMode::one_shot;
                break;
            case hash("loop_continuous"):
                loopMode = SfzLoopMode::loop_continuous;
                break;
            case hash("loop_sustain"):
                loopMode = SfzLoopMode::loop_sustain;
                break;
            default:
                DBG("Unkown loop mode:" << std::string(opcode.value));
        }
        break;
    case hash("loopend"):
    case hash("loop_end"): setRangeEndFromOpcode(opcode, loopRange, SfzDefault::loopRange); break;
    case hash("loopstart"):
    case hash("loop_start"): setRangeStartFromOpcode(opcode, loopRange, SfzDefault::loopRange); break;

    // Instrument settings: voice lifecycle
    case hash("group"): setValueFromOpcode(opcode, group, SfzDefault::groupRange); break;
    case hash("offby"):
    case hash("off_by"): setValueFromOpcode(opcode, offBy, SfzDefault::groupRange); break;
    case hash("off_mode"):
        switch(hash(opcode.value))
        {
            case hash("fast"):
                offMode = SfzOffMode::fast;
                break;
            case hash("normal"):
                offMode = SfzOffMode::normal;
                break;
            default:
                DBG("Unkown off mode:" << std::string(opcode.value));
        }
        break;
    // Region logic: key mapping
    case hash("lokey"): setRangeStartFromOpcode(opcode, keyRange, SfzDefault::keyRange); break;
    case hash("hikey"): setRangeEndFromOpcode(opcode, keyRange, SfzDefault::keyRange); break;
    case hash("key"):
        setRangeStartFromOpcode(opcode, keyRange, SfzDefault::keyRange);
        setRangeEndFromOpcode(opcode, keyRange, SfzDefault::keyRange);
        setValueFromOpcode(opcode, pitchKeycenter, SfzDefault::keyRange);
        break;
    case hash("lovel"): setRangeStartFromOpcode(opcode, velocityRange, SfzDefault::velocityRange); break;
    case hash("hivel"): setRangeEndFromOpcode(opcode, velocityRange, SfzDefault::velocityRange); break;
        
    // Region logic: MIDI conditions
    case hash("lochan"): setRangeStartFromOpcode(opcode, channelRange, SfzDefault::channelRange); break;
    case hash("hichan"): setRangeEndFromOpcode(opcode, channelRange, SfzDefault::channelRange); break;
    case hash("lobend"): setRangeStartFromOpcode(opcode, bendRange, SfzDefault::bendRange); break;
    case hash("hibend"): setRangeEndFromOpcode(opcode, bendRange, SfzDefault::bendRange); break;
    case hash("locc"): 
        if (opcode.parameter) 
            setRangeStartFromOpcode(opcode, ccConditions[*opcode.parameter], SfzDefault::ccRange); 
        break;
    case hash("hicc"):
        if (opcode.parameter) 
            setRangeEndFromOpcode(opcode, ccConditions[*opcode.parameter], SfzDefault::ccRange); 
        break;
    case hash("sw_lokey"): setRangeStartFromOpcode(opcode, keyswitchRange, SfzDefault::keyRange); break;
    case hash("sw_hikey"): setRangeEndFromOpcode(opcode, keyswitchRange, SfzDefault::keyRange); break;
    case hash("sw_last"): setValueFromOpcode(opcode, keyswitch, SfzDefault::keyRange); break;
    case hash("sw_down"): setValueFromOpcode(opcode, keyswitchDown, SfzDefault::keyRange); break;
    case hash("sw_up"): setValueFromOpcode(opcode, keyswitchUp, SfzDefault::keyRange); break;
    case hash("sw_previous"): setValueFromOpcode(opcode, previousNote, SfzDefault::keyRange); break;
    case hash("sw_vel"):
        switch(hash(opcode.value))
        {
            case hash("current"):
                velocityOverride = SfzVelocityOverride::current;
                break;
            case hash("previous"):
                velocityOverride = SfzVelocityOverride::previous;
                break;
            default:
                DBG("Unknown velocity mode: " << std::string(opcode.value));
        }
        break;
    // Region logic: internal conditions
    case hash("lochanaft"): setRangeStartFromOpcode(opcode, aftertouchRange, SfzDefault::aftertouchRange); break;
    case hash("hichanaft"): setRangeEndFromOpcode(opcode, aftertouchRange, SfzDefault::aftertouchRange); break;
    case hash("lobpm"): setRangeStartFromOpcode(opcode, bpmRange, SfzDefault::bpmRange); break;
    case hash("hibpm"): setRangeEndFromOpcode(opcode, bpmRange, SfzDefault::bpmRange); break;
    case hash("lorand"): setRangeStartFromOpcode(opcode, randRange, SfzDefault::randRange); break;
    case hash("hirand"): setRangeEndFromOpcode(opcode, randRange, SfzDefault::randRange); break;
    case hash("seq_length"): setValueFromOpcode(opcode, sequenceLength, SfzDefault::sequenceRange); break;
    case hash("seq_position"): setValueFromOpcode(opcode, sequencePosition, SfzDefault::sequenceRange); break;
    // Region logic: triggers
    case hash("trigger"):
        switch(hash(opcode.value))
        {
            case hash("attack"):
                trigger = SfzTrigger::attack;
                break;
            case hash("first"):
                trigger = SfzTrigger::first;
                break;
            case hash("legato"):
                trigger = SfzTrigger::legato;
                break;
            case hash("release"):
                trigger = SfzTrigger::release;
                break;
            case hash("release_key"):
                trigger = SfzTrigger::release_key;
                break;
            default:
                DBG("Unknown trigger mode: " << std::string(opcode.value));
        }
        break;
    case hash("on_locc"): 
        if (opcode.parameter) 
            setRangeStartFromOpcode(opcode, ccTriggers[*opcode.parameter], SfzDefault::ccRange); 
        break;
    case hash("on_hicc"):
        if (opcode.parameter) 
            setRangeEndFromOpcode(opcode, ccTriggers[*opcode.parameter], SfzDefault::ccRange); 
        break;

    // Performance parameters: amplifier
    case hash("volume"): setValueFromOpcode(opcode, volume, SfzDefault::volumeRange); break;
    // case hash("volume_oncc"): 
    //     if (opcode.parameter) 
    //         setValueFromOpcode(opcode, volumeCCTriggers[*opcode.parameter], SfzDefault::volumeCCRange); 
    //     break;
    case hash("amplitude"): setValueFromOpcode(opcode, amplitude, SfzDefault::amplitudeRange); break;
    case hash("amplitude_cc"):
    case hash("amplitude_oncc"): setCCPairFromOpcode(opcode, amplitudeCC, SfzDefault::amplitudeRange); break;
    case hash("pan"): setValueFromOpcode(opcode, pan, SfzDefault::panRange); break;
    case hash("pan_oncc"): setCCPairFromOpcode(opcode, panCC, SfzDefault::panCCRange); break;
    case hash("position"): setValueFromOpcode(opcode, position, SfzDefault::positionRange); break;
    case hash("position_oncc"): setCCPairFromOpcode(opcode, positionCC, SfzDefault::positionCCRange); break;
    case hash("width"): setValueFromOpcode(opcode, width, SfzDefault::widthRange); break;
    case hash("width_oncc"): setCCPairFromOpcode(opcode, widthCC, SfzDefault::widthCCRange); break;
    case hash("amp_keycenter"): setValueFromOpcode(opcode, ampKeycenter, SfzDefault::keyRange); break;
    case hash("amp_keytrack"): setValueFromOpcode(opcode, ampKeytrack, SfzDefault::ampKeytrackRange); break;
    case hash("amp_veltrack"): setValueFromOpcode(opcode, ampVeltrack, SfzDefault::ampVeltrackRange); break;
    case hash("amp_random"): setValueFromOpcode(opcode, ampRandom, SfzDefault::ampRandomRange); break;
    case hash("amp_velcurve_"):
        if (opcode.parameter && withinRange(SfzDefault::ccRange, *opcode.parameter))
        {
            if (auto value = readOpcode(opcode.value, SfzDefault::ampVelcurveRange); value)
                velocityPoints.emplace_back(*opcode.parameter, *value);
        }
        break;
    case hash("xfin_lokey"): setRangeStartFromOpcode(opcode, crossfadeKeyInRange, SfzDefault::keyRange); break;
    case hash("xfin_hikey"): setRangeEndFromOpcode(opcode, crossfadeKeyInRange, SfzDefault::keyRange); break;
    case hash("xfout_lokey"): setRangeStartFromOpcode(opcode, crossfadeKeyOutRange, SfzDefault::keyRange); break;
    case hash("xfout_hikey"): setRangeEndFromOpcode(opcode, crossfadeKeyOutRange, SfzDefault::keyRange); break;
    case hash("xfin_lovel"): setRangeStartFromOpcode(opcode, crossfadeVelInRange, SfzDefault::velocityRange); break;
    case hash("xfin_hivel"): setRangeEndFromOpcode(opcode, crossfadeVelInRange, SfzDefault::velocityRange); break;
    case hash("xfout_lovel"): setRangeStartFromOpcode(opcode, crossfadeVelOutRange, SfzDefault::velocityRange); break;
    case hash("xfout_hivel"): setRangeEndFromOpcode(opcode, crossfadeVelOutRange, SfzDefault::velocityRange); break;
    case hash("xf_keycurve"):
        switch (hash(opcode.value))
        {
        case hash("power"):
            crossfadeKeyCurve = SfzCrossfadeCurve::power;
            break;
        case hash("gain"):
            crossfadeKeyCurve = SfzCrossfadeCurve::gain;
            break;
        default:
            DBG("Unknown crossfade power curve: " << std::string(opcode.value));
        }
        break;
    case hash("xf_velcurve"):
        switch (hash(opcode.value))
        {
        case hash("power"):
            crossfadeVelCurve = SfzCrossfadeCurve::power;
            break;
        case hash("gain"):
            crossfadeVelCurve = SfzCrossfadeCurve::gain;
            break;
        default:
            DBG("Unknown crossfade power curve: " << std::string(opcode.value));
        }
        break;

    // Performance parameters: pitch
    case hash("pitch_keycenter"): setValueFromOpcode(opcode, pitchKeycenter, SfzDefault::keyRange); break;
    case hash("pitch_keytrack"): setValueFromOpcode(opcode, pitchKeytrack, SfzDefault::pitchKeytrackRange); break;
    case hash("pitch_veltrack"): setValueFromOpcode(opcode, pitchVeltrack, SfzDefault::pitchVeltrackRange); break;
    case hash("pitch_random"): setValueFromOpcode(opcode, pitchRandom, SfzDefault::pitchRandomRange); break;
    case hash("transpose"): setValueFromOpcode(opcode, transpose, SfzDefault::transposeRange); break;
    case hash("tune"): setValueFromOpcode(opcode, tune, SfzDefault::tuneRange); break;

    // Amplitude Envelope
    case hash("ampeg_attack"): setValueFromOpcode(opcode, amplitudeEG.attack, SfzDefault::egTimeRange); break;
    case hash("ampeg_decay"): setValueFromOpcode(opcode, amplitudeEG.decay, SfzDefault::egTimeRange); break;
    case hash("ampeg_delay"): setValueFromOpcode(opcode, amplitudeEG.delay, SfzDefault::egTimeRange); break;
    case hash("ampeg_hold"): setValueFromOpcode(opcode, amplitudeEG.hold, SfzDefault::egTimeRange); break;
    case hash("ampeg_release"): setValueFromOpcode(opcode, amplitudeEG.release, SfzDefault::egTimeRange); break;
    case hash("ampeg_start"): setValueFromOpcode(opcode, amplitudeEG.start, SfzDefault::egPercentRange); break;
    case hash("ampeg_sustain"): setValueFromOpcode(opcode, amplitudeEG.sustain, SfzDefault::egPercentRange); break;
    case hash("ampeg_vel2attack"): setValueFromOpcode(opcode, amplitudeEG.vel2attack, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_vel2decay"): setValueFromOpcode(opcode, amplitudeEG.vel2decay, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_vel2delay"): setValueFromOpcode(opcode, amplitudeEG.vel2delay, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_vel2hold"): setValueFromOpcode(opcode, amplitudeEG.vel2hold, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_vel2release"): setValueFromOpcode(opcode, amplitudeEG.vel2release, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_vel2sustain"): setValueFromOpcode(opcode, amplitudeEG.vel2sustain, SfzDefault::egOnCCPercentRange); break;
    case hash("ampeg_attack_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccAttack, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_decay_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccDecay, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_delay_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccDelay, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_hold_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccHold, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_release_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccRelease, SfzDefault::egOnCCTimeRange); break;
    case hash("ampeg_start_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccStart, SfzDefault::egOnCCPercentRange); break;
    case hash("ampeg_sustain_oncc"): setCCPairFromOpcode(opcode, amplitudeEG.ccSustain, SfzDefault::egOnCCPercentRange); break;
    // Ignored opcodes
    case hash("ampeg_depth"):
    case hash("ampeg_vel2depth"):
        break;
    default:
        std::string opcodeStr { opcode.opcode.begin(), opcode.opcode.end() };
        unknownOpcodes.push_back(opcodeStr);
    }

    // Unsupported opcodes
    
}

String SfzRegion::stringDescription() const noexcept
{
    String returnedString { sample };
    if (keyRange != SfzDefault::keyRange)               { returnedString << " keyRange: " << printRange(keyRange); }
    if (velocityRange != SfzDefault::velocityRange)     { returnedString << " velocityRange: " << printRange(velocityRange); }
    if (volume != SfzDefault::volume)                   { returnedString << " volume: " << volume; }
    if (pan != SfzDefault::pan)                         { returnedString << " pan: " << pan; }
    if (trigger == SfzTrigger::release)                 { returnedString << " (Release)"; }
    return returnedString;
}

bool SfzRegion::registerNoteOn(int channel, int noteNumber, uint8_t velocity, float randValue)
{
    // You have to call prepare before sending notes to the region
    jassert(prepared);

    const bool chanOk = withinRange(channelRange, channel);
    if (!chanOk)
        return false;

    if (withinRange(keyswitchRange, noteNumber))
    {
        if (keyswitch)
        {
            if (*keyswitch == noteNumber)
                keySwitched = true;
            else
                keySwitched = false;
        }

        if (keyswitchDown && *keyswitchDown == noteNumber)
            keySwitched = true;

        if (keyswitchUp && *keyswitchUp == noteNumber)
            keySwitched = false;
    }

    const bool keyOk = withinRange(keyRange, noteNumber);
    if (keyOk)
    {
        // Update the number of notes playing for the region
        activeNotesInRange++;

        // Sequence activation
        sequenceCounter += 1;
        if ((sequenceCounter % sequenceLength) == sequencePosition - 1)
            sequenceSwitched = true;
        else
            sequenceSwitched = false;

        // Velocity memory for release_key and for sw_vel=previous
        if (trigger == SfzTrigger::release_key || velocityOverride == SfzVelocityOverride::previous)
            lastNoteVelocities[noteNumber] = velocity;

        if (previousNote)
        {
            if ( *previousNote == noteNumber)
                previousKeySwitched = true;
            else
                previousKeySwitched = false;
        }
    }

    if (!isSwitchedOn())
        return false;

    if (previousNote && !(previousKeySwitched && noteNumber != *previousNote))
        return false;

    const bool velOk = withinRange(velocityRange, velocity);
    const bool randOk = withinRange(randRange, randValue);
    const bool firstLegatoNote = (trigger == SfzTrigger::first && activeNotesInRange == 0);
    const bool attackTrigger = (trigger == SfzTrigger::attack);
    const bool notFirstLegatoNote = (trigger == SfzTrigger::legato && activeNotesInRange > 0);
    return keyOk && velOk && chanOk && randOk && (attackTrigger || firstLegatoNote || notFirstLegatoNote);
}

bool SfzRegion::registerNoteOff(int channel, int noteNumber, uint8_t velocity, float randValue)
{
    // You have to call prepare before sending notes to the region
    jassert(prepared);

    const bool chanOk = withinRange(channelRange, channel);
    if (!chanOk)
        return false;

    if (withinRange(keyswitchRange, noteNumber))
    {
        if (keyswitchDown && *keyswitchDown == noteNumber)
            keySwitched = false;

        if (keyswitchUp && *keyswitchUp == noteNumber)
            keySwitched = true;
    }

    const bool keyOk = withinRange(keyRange, noteNumber);
    // Update the number of notes playing for the region
    if (keyOk)
        activeNotesInRange--;

    if (!isSwitchedOn())
        return false;
    
    const bool randOk = withinRange(randRange, randValue);
    const bool releaseTrigger = (trigger == SfzTrigger::release || trigger == SfzTrigger::release_key);
    return keyOk && chanOk && randOk && releaseTrigger;
}

bool SfzRegion::registerCC(int channel, int ccNumber, uint8_t ccValue)
{
    // You have to prepare the region before calling this function
    jassert(prepared);
    if (withinRange(ccConditions.getWithDefault(ccNumber), ccValue))
        ccSwitched[ccNumber] = true;
    else
        ccSwitched[ccNumber] = false;

    if (ccTriggers.contains(ccNumber) && withinRange(ccTriggers.at(ccNumber), ccValue))
        return true;
    else
        return false;
}

void SfzRegion::registerPitchWheel(int channel, int pitch)
{
    // You have to prepare the region before calling this function
    jassert(prepared);
    if (withinRange(bendRange, pitch))
        pitchSwitched = true;
    else
        pitchSwitched = false;
}

void SfzRegion::registerAftertouch(int channel, uint8_t aftertouch)
{
    // You have to prepare the region before calling this function
    jassert(prepared);
    if (withinRange(aftertouchRange, aftertouch))
        aftertouchSwitched = true;
    else
        aftertouchSwitched = false;
}

void SfzRegion::registerTempo(float secondsPerQuarter)
{
    // You have to prepare the region before calling this function
    jassert(prepared);
    const float bpm = 60.0f / secondsPerQuarter;
    if (withinRange(bpmRange, bpm))
        bpmSwitched = true;
    else
        bpmSwitched = false;
}

bool SfzRegion::prepare()
{
    prepared = false;

    if (!isGenerator())
    {
        filePool.preload(sample, offset + offsetRandom);
        auto reader = filePool.createReaderFor(sample);
        if (reader == nullptr)
        {
            DBG("[Prepare region] Error creating reader for " << sample);
            return false;
        }

        sampleRate = reader->sampleRate;
        if (sampleEnd == SfzDefault::sampleEndRange.getEnd())
            sampleEnd = static_cast<uint32_t>(reader->lengthInSamples);
        numChannels = reader->numChannels;

        if (reader->metadataValues.containsKey("Loop0Start") && reader->metadataValues.containsKey("Loop0End") && loopRange == SfzDefault::loopRange)
        {
            // DBG("Looping between " << reader->metadataValues["Loop0Start"] << " and " << reader->metadataValues["Loop0End"]);
            loopRange.setStart(static_cast<uint32_t>(reader->metadataValues["Loop0Start"].getLargeIntValue()));
            loopRange.setEnd(static_cast<uint32_t>(reader->metadataValues["Loop0End"].getLargeIntValue()));
        }
    }

    if (sampleCount)
        loopMode = SfzLoopMode::one_shot;

    if (sampleEnd == 0 || sample == "")
        sample = "*silence";

    if (trigger == SfzTrigger::release_key)
    {
        for (auto& vel: lastNoteVelocities)
            vel = 0;
    }

    addEndpointsToVelocityCurve();
    checkInitialConditions();
    prepared = true;
    return true;
}

void SfzRegion::addEndpointsToVelocityCurve()
{
    if (velocityPoints.size() > 0)
    {
        std::sort(velocityPoints.begin(), velocityPoints.end(), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });
        if (ampVeltrack > 0)
        {
            if (velocityPoints.back().first != SfzDefault::velocityRange.getEnd())
                velocityPoints.push_back(std::make_pair<int, float>(127, 1.0f));
            if (velocityPoints.front().first != SfzDefault::velocityRange.getStart())
                velocityPoints.insert(velocityPoints.begin(), std::make_pair<int, float>(0, 0.0f));
        }
        else
        {
            if (velocityPoints.front().first != SfzDefault::velocityRange.getEnd())
                velocityPoints.insert(velocityPoints.begin(), std::make_pair<int, float>(127, 0.0f));
            if (velocityPoints.back().first != SfzDefault::velocityRange.getStart())
                velocityPoints.push_back(std::make_pair<int, float>(0, 1.0f));
        }        
    }
}

void SfzRegion::checkInitialConditions()
{
    
    for (int ccIdx = 0; ccIdx < 128; ++ccIdx)
    {
        if (ccConditions.getWithDefault(ccIdx).getStart() > 0)
            ccSwitched[ccIdx] = false;
    }

    if (!bendRange.contains(SfzDefault::bend))
        pitchSwitched = false;

    if (!aftertouchRange.contains(SfzDefault::aftertouch))
        aftertouchSwitched = false;

    if (!bpmRange.contains(SfzDefault::bpm))
        bpmSwitched = false;

    if (sequencePosition > 1)
        sequenceSwitched = false;
    
    if (keyswitch)
        keySwitched = false;

    if (keyswitchDown)
        keySwitched = false;
        
    if (previousNote)
        previousKeySwitched = false;
}

bool SfzRegion::isStereo() const noexcept
{
    return numChannels == 2;
}

float SfzRegion::velocityGain(uint8_t velocity) const noexcept
{
    float gaindB { 0.0 };
    if (velocityPoints.size() > 0)
    {
        auto after = std::find_if(velocityPoints.begin(), velocityPoints.end(), [velocity](auto& val) { return val.first >= velocity; });
        auto before = after == velocityPoints.begin() ? velocityPoints.begin() : after - 1;
        // Linear interpolation
        float relativePositionInSegment { static_cast<float>(velocity - before->first) / (after->first - before->first) };
        float segmentEndpoints { after->second - before->second };
        gaindB =  Decibels::gainToDecibels(relativePositionInSegment * segmentEndpoints);
    }
    else
    {
        float floatVelocity { static_cast<float>(velocity)/127 };
        if (ampVeltrack > 0)
            gaindB =  40 * std::log(floatVelocity) / std::log(10.0f);
        else
            gaindB =  40 * std::log(1-floatVelocity) / std::log(10.0f);
    }
    gaindB *= std::abs(ampVeltrack) / SfzDefault::ampVeltrackRange.getEnd();
    return Decibels::decibelsToGain(gaindB);
}

bool SfzRegion::isSwitchedOn() const noexcept
{
    return  keySwitched 
            && previousKeySwitched 
            && sequenceSwitched 
            && pitchSwitched 
            && bpmSwitched 
            && aftertouchSwitched 
            && std::all_of(ccSwitched.cbegin(), ccSwitched.cend(), [](auto& sw) { return sw; });
}