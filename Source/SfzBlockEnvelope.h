#pragma once
#include <vector>
#include <algorithm>
#include <functional>
#include <cstdint>

template<class OutputType = float, class InputType = uint8_t>
class SfzBlockEnvelope
{
public:
    SfzBlockEnvelope(int maximum, OutputType initialValue = static_cast<OutputType>(0.0))
    : currentValue(initialValue)
    {
        reserve(maximum);
    }

    void reserve(int maximum)
    {
        events.reserve(maximumEvents);
        maximumEvents = maximum;
    }

    void addEvent(InputType value, int timestamp)
    {
        if (events.size() < maximumEvents)
            events.push_back({ timestamp, value });
    }

    void getEnvelope(OutputType* output, int numSamples)
    {
        if (events.empty())
        {
            std::fill(output, output + numSamples, currentValue);
            return;
        }

        std::sort(events.begin(), events.end(), EventComparator());
        int eventIndex { 0 };

        while (events[eventIndex].timestamp == 0 && eventIndex < events.size())
        {
            currentValue = transform(events[eventIndex].value);
            eventIndex++;
        }

        if (eventIndex == events.size())
        {
            std::fill(output, output + numSamples, currentValue);
            clearEvents();
            return;
        }

        int sampleIndex { 0 };
        int numSteps { events[eventIndex].timestamp };
        OutputType step { (transform(events[eventIndex].value) - currentValue) / numSteps };

        while (sampleIndex < numSamples)
        {
            if (numSteps == 0)
            {
                while (events[eventIndex].timestamp == sampleIndex && eventIndex < events.size())
                {
                    currentValue = transform(events[eventIndex].value);
                    eventIndex++;
                }
                
                if (eventIndex == events.size())
                {
                    std::fill(output + sampleIndex, output + numSamples, currentValue);
                    clearEvents();
                    return;
                }
                else
                {
                    numSteps = events[eventIndex].timestamp - sampleIndex;
                    step = (transform(events[eventIndex].value) - currentValue) / numSteps;
                }
            }

            output[sampleIndex] = currentValue;
            numSteps--;
            sampleIndex++;
            currentValue += step;
        }
        // Backtrack if we get here because it means we went 1 step too far out
        currentValue -= step;
        clearEvents();
    }
    void clearEvents()
    {
        events.clear();
    }
    void setFunction(std::function<OutputType(InputType)> function)
    {
        transform = function;
    }
private:
    struct Event
    {
        int timestamp;
        InputType value;
    };
    struct EventComparator
    {
        bool operator()(const Event& lhs, const Event& rhs) const { return lhs.timestamp < rhs.timestamp; }
    };
    std::vector<Event> events;
    int maximumEvents { 0 };
    OutputType currentValue { static_cast<OutputType>(0.0) };
    std::function<OutputType(InputType)> transform { [](InputType in){ return static_cast<OutputType>(in); } };
};