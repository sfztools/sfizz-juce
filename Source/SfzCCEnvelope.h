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
    EnvelopeEvent(uint32_t timestamp, uint8_t ccValue) 
    : timestamp(timestamp), ccValue(ccValue) {}
    uint32_t timestamp;
    uint8_t ccValue;
};

class SfzCCEnvelope
{
public:
    SfzCCEnvelope(int maximumSize = config::defaultSamplesPerBlock)
    : fifo(maximumSize)
    {
        points.reserve(maximumSize);
        smoothedValue.setCurrentAndTargetValue(0.0f);
    }

    void setSize(int maximumSize)
    {
        fifo.reset();
        points.reserve(maximumSize);
        fifo.setTotalSize(maximumSize);
    }

    void setTransform(std::function<float(float)> transform) noexcept
    {
        transformValue = transform;
    }

    void reset(uint8_t ccValue = SfzDefault::cc) noexcept
    {
        localTime = 0;
        smoothedValue.setCurrentAndTargetValue(normalizeCC(ccValue));
    }

    void addEvent(int timestamp, uint8_t ccValue) noexcept
    {
        if (fifo.getFreeSpace() == 0)
            return;

        AbstractFifo::ScopedWrite writer { fifo, 1 };
        if (writer.blockSize1 == 1)
            points.emplace(begin(points) + writer.startIndex1, timestamp, ccValue);
        else
            points.emplace(begin(points) + writer.startIndex2, timestamp, ccValue);
    };

    float getNextValue() noexcept
    {
        if (!smoothedValue.isSmoothing() && fifo.getNumReady() > 0)
        {
            auto [timestamp, ccValue] = readEventFromFifo();
            auto rampDuration = timestamp > localTime ? timestamp - localTime : 0;
            smoothedValue.reset(rampDuration);
            smoothedValue.setTargetValue(normalizeCC(ccValue));
        }

        localTime++;
        return transformValue(smoothedValue.getNextValue());
    }

private:
    EnvelopeEvent readEventFromFifo() noexcept
    {
        AbstractFifo::ScopedRead reader { fifo, 1 };
        if (reader.blockSize1 == 1)
            return points[reader.startIndex1];
        else
            return points[reader.startIndex2];
    };

    std::function<float(float)> transformValue { [] (float ccValue) -> float { return ccValue; } };
    std::vector<EnvelopeEvent> points;
    AbstractFifo fifo;
    SmoothedValue<float, ValueSmoothingTypes::Linear> smoothedValue;
    uint32_t localTime { 0 };
};


// Constant power pan rule
inline void applyPanToSample(float pan, float& left, float& right)
{
    const float normalizedPan = pan / SfzDefault::panRange.getEnd();
    const float circlePan = MathConstants<float>::pi * (normalizedPan + 1) / 4;
    if (pan < 0) 
        std::swap(left, right);
    left *= std::cos(circlePan);
    right *= std::sin(circlePan);
}

inline void applyWidthAndPositionToSample(float width, float position, float& left, float& right)
{
    const float normalizedWidth = width / SfzDefault::widthRange.getEnd();
    const float circleWidth = MathConstants<float>::pi * (normalizedWidth + 1) / 4;
    const float normalizedPosition = position / SfzDefault::positionRange.getEnd();
    const float circlePosition = MathConstants<float>::pi * (normalizedPosition + 1) / 4;
    float mid = (left + right) / MathConstants<float>::sqrt2;
    float side = (left - right) / MathConstants<float>::sqrt2;
    mid *= std::cos(circleWidth);
    side *= std::sin(circleWidth);
    left = (mid + side) * std::cos(circlePosition) / MathConstants<float>::sqrt2;
    right = (mid - side) * std::sin(circlePosition) / MathConstants<float>::sqrt2;
}
