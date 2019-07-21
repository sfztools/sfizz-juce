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
    EnvelopeEvent(int timestamp, uint8_t ccValue) 
    : timestamp(timestamp), ccValue(ccValue) {}
    int timestamp;
    uint8_t ccValue;
};

class SfzCCEnvelope
{
public:
    SfzCCEnvelope(int maximumSize = config::defaultSamplesPerBlock)
    {
        setSize(maximumSize);
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

    void clear() noexcept
    {
        points.clear();
        lastTimestamp = 0;
        readIndex = 0;
    }

    void addEvent(int timestamp, uint8_t ccValue) noexcept
    {
        if (getFreeSpace() == 0)
            return;

        points.emplace_back(timestamp, ccValue);
    };

    float getNextValue() noexcept
    {
        if (!smoothedValue.isSmoothing() && getNumReady() > 0)
        {
            const auto delay = std::max(0, points[readIndex].timestamp - lastTimestamp);
            smoothedValue.reset(delay);
            smoothedValue.setTargetValue(normalizeCC(points[readIndex].ccValue));
            lastTimestamp = points[readIndex].timestamp;
            readIndex = (readIndex + 1);
        }
        return transformValue(smoothedValue.getNextValue());
    }

private:
    int getNumReady() const noexcept
    {
        return static_cast<int>(points.size()) - readIndex;
    }

    int getFreeSpace() const noexcept
    {
        return static_cast<int>(points.capacity() - points.size());
    }

    std::function<float(float)> transformValue { [] (float ccValue) -> float { return ccValue; } };
    std::vector<EnvelopeEvent> points;
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothedValue;
    int readIndex { 0 };
    int writeIndex { 0 };
    int size { 0 };
    int lastTimestamp { 0 };
};