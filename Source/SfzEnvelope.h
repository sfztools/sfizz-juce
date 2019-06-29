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
#include <optional>

inline float ccSwitchedValue(const CCValueArray& ccValues, const std::optional<CCValuePair>& ccSwitch, float value) noexcept
{
    if (ccSwitch)
        return value + ccSwitch->second * normalizeCC(ccValues[ccSwitch->first]);
    else
        return value;
}

struct SfzEnvelopeGeneratorDescription
{
    float attack        { SfzDefault::attack };
    float decay         { SfzDefault::decay };
    float delay         { SfzDefault::delayEG };
    float hold          { SfzDefault::hold };
    float release       { SfzDefault::release };
    float start         { SfzDefault::start };
    float sustain       { SfzDefault::sustain };
    int   depth         { SfzDefault::depth };
    float vel2attack    { SfzDefault::attack };
    float vel2decay     { SfzDefault::decay };
    float vel2delay     { SfzDefault::delayEG };
    float vel2hold      { SfzDefault::hold };
    float vel2release   { SfzDefault::release };
    float vel2sustain   { SfzDefault::vel2sustain };
    int   vel2depth     { SfzDefault::depth };

    std::optional<CCValuePair> ccAttack;
    std::optional<CCValuePair> ccDecay;
    std::optional<CCValuePair> ccDelay;
    std::optional<CCValuePair> ccHold;
    std::optional<CCValuePair> ccRelease;
    std::optional<CCValuePair> ccStart;
    std::optional<CCValuePair> ccSustain;

    float getAttack(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccAttack, attack) + normalizeCC(velocity)*vel2attack;
    }
    float getDecay(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccDecay, decay) + normalizeCC(velocity)*vel2decay;
    }
    float getDelay(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccDelay, delay) + normalizeCC(velocity)*vel2delay;
    }
    float getHold(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccHold, hold) + normalizeCC(velocity)*vel2hold;
    }
    float getRelease(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccRelease, release) + normalizeCC(velocity)*vel2release;
    }
    float getStart(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccStart, start);
    }
    float getSustain(const CCValueArray &ccValues, uint8_t velocity) const noexcept
    {
        return ccSwitchedValue(ccValues, ccSustain, sustain) + normalizeCC(velocity)*vel2sustain;
    }
};

class SfzEnvelopeGeneratorValue
{
public:
    SfzEnvelopeGeneratorValue() noexcept
    {
        attackEnvelopeValue.reset(1);
        attackEnvelopeValue.setCurrentAndTargetValue(0.0f);
        attackEnvelopeValue.setTargetValue(1.0f);
        decayEnvelopeValue.reset(1);
        decayEnvelopeValue.setCurrentAndTargetValue(1.0f);
        decayEnvelopeValue.setTargetValue(config::virtuallyZero);
        releaseEnvelopeValue.reset(1);
        releaseEnvelopeValue.setCurrentAndTargetValue(1.0f);
        releaseEnvelopeValue.setTargetValue(config::virtuallyZero);
    }
    void setSampleRate(double rate) noexcept { sampleRate = rate; }

    void prepare(const SfzEnvelopeGeneratorDescription& egDescription, const CCValueArray& ccValues, uint8_t velocity, uint32_t additionalDelay = 0) noexcept
    {
        auto secondsToSamples = [this](auto timeInSeconds) { 
            return static_cast<int>(timeInSeconds * sampleRate);
        };
        state = EGState::attack;
        remainingSamplesBeforeRelease = 0;
        remainingDelaySamples = additionalDelay + secondsToSamples(egDescription.getDelay(ccValues, velocity));
        remainingHoldSamples = secondsToSamples(egDescription.getHold(ccValues, velocity));
        sustainGain = normalizePercents(egDescription.getSustain(ccValues, velocity));
        attackEnvelopeValue.reset(secondsToSamples(egDescription.getAttack(ccValues, velocity)));
        attackEnvelopeValue.setCurrentAndTargetValue(0.0f);
        attackEnvelopeValue.setTargetValue(1.0f);
        releaseEnvelopeValue.reset(secondsToSamples(egDescription.getRelease(ccValues, velocity)));
        releaseEnvelopeValue.setCurrentAndTargetValue(1.0f);
        releaseEnvelopeValue.setTargetValue(config::virtuallyZero);
        decayEnvelopeValue.reset(secondsToSamples(egDescription.getDecay(ccValues, velocity)));
        decayEnvelopeValue.setCurrentAndTargetValue(1.0f);
        decayEnvelopeValue.setTargetValue(config::virtuallyZero);
    }

    enum class EGState { attack, release };

    bool isSmoothing() noexcept
    {
        return releaseEnvelopeValue.isSmoothing();
    }

    float getNextValue() noexcept
    {
        float envelopeGain { config::virtuallyZero };

        if (state == EGState::attack)
        {
            if (remainingDelaySamples > 0)
            {
                // We're at 0 already
                remainingDelaySamples--;
            }
            
            if (remainingDelaySamples == 0 && attackEnvelopeValue.isSmoothing())
            {
                envelopeGain = attackEnvelopeValue.getNextValue();
            }

            if (remainingDelaySamples == 0 && !attackEnvelopeValue.isSmoothing() && remainingHoldSamples > 0)
            {
                envelopeGain = 1.0f;
                remainingHoldSamples--;
            }

            if (remainingDelaySamples == 0 && !attackEnvelopeValue.isSmoothing() && remainingHoldSamples == 0)
            {
                envelopeGain = decayEnvelopeValue.getNextValue() * (1 - sustainGain) + sustainGain;
            }                
        }

        if (state == EGState::release)
        {
            envelopeGain = decayEnvelopeValue.getNextValue() * (1 - sustainGain) + sustainGain;
            envelopeGain *= attackEnvelopeValue.getCurrentValue();
            if (remainingSamplesBeforeRelease > 0)
                remainingSamplesBeforeRelease--;
            else
                envelopeGain *= releaseEnvelopeValue.getNextValue();
        }

        return envelopeGain;
    }

    void release(uint32_t delay = 0, bool fastRelease = false) noexcept
    {
        if (fastRelease)
            releaseEnvelopeValue.reset(static_cast<int>(config::fastReleaseDuration * sampleRate));
        
        remainingSamplesBeforeRelease = delay;
        state = EGState::release;
    }

private:
    uint32_t remainingDelaySamples { 0 };
    EGState state { EGState::attack };
    SmoothedValue<float, ValueSmoothingTypes::Linear> attackEnvelopeValue;
    uint32_t remainingHoldSamples { 0 };
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> decayEnvelopeValue;
    float sustainGain { 1.0 };
    uint32_t remainingSamplesBeforeRelease { 0 };
    SmoothedValue<float, ValueSmoothingTypes::Multiplicative> releaseEnvelopeValue;
    double sampleRate { config::defaultSampleRate };
};