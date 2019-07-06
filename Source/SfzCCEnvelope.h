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

struct EnvelopeEvent
{
    EnvelopeEvent(int delay, uint8_t ccValue) 
    : delay(delay), ccValue(ccValue) {}
    int delay;
    uint8_t ccValue;
};

class SfzCCEnvelope
{
public:
    SfzCCEnvelope(int maximumSize = config::defaultSamplesPerBlock)
    : size(maximumSize)
    {
        points.reserve(maximumSize);
        smoothedValue.setCurrentAndTargetValue(0.0f);
    }

    void setSize(int maximumSize)
    {
        size = maximumSize;
        points.reserve(maximumSize);
    }

    void setTransform(std::function<float(float)> transform) noexcept
    {
        transformValue = transform;
    }

    void reset(uint8_t ccValue = SfzDefault::cc) noexcept
    {
        smoothedValue.setCurrentAndTargetValue(normalizeCC(ccValue));
    }

    void addEvent(int delay, uint8_t ccValue) noexcept
    {
        if (getFreeSpace() == 0)
            return;

        points[writeIndex] = EnvelopeEvent(delay, ccValue);
        writeIndex = (writeIndex + 1) % size;
    };

    float getNextValue() noexcept
    {
        if (!smoothedValue.isSmoothing() && getNumReady() > 0)
        {
            smoothedValue.reset(points[readIndex].delay);
            smoothedValue.setTargetValue(normalizeCC(points[readIndex].ccValue));
            readIndex = (readIndex + 1) % size;
        }
        return transformValue(smoothedValue.getNextValue());
    }

private:
    int getNumReady() const noexcept
    {
        if (writeIndex >= readIndex)
            return writeIndex - readIndex;
        else
            return size - readIndex + writeIndex;
    }

    int getFreeSpace() const noexcept
    {
        if (writeIndex >= readIndex)
            return size - writeIndex + readIndex;
        else
            return readIndex - writeIndex;
    }

    std::function<float(float)> transformValue { [] (float ccValue) -> float { return ccValue; } };
    std::vector<EnvelopeEvent> points;
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothedValue;
    int readIndex { 0 };
    int writeIndex { 0 };
    int size { 0 };
};