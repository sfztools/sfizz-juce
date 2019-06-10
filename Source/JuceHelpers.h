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
#include <functional>
#include <type_traits>
/**
 * A simple callback holder watching for the visibility of a JUCE component
 * Instantiate with a component and a callback and let it handle the rest!
 */
template<class T>
struct SimpleVisibilityWatcher: public ComponentMovementWatcher
{
    static_assert(std::is_base_of<Component, T>::value);
    SimpleVisibilityWatcher() = delete;
    SimpleVisibilityWatcher(T& component, std::function<void()> callback)
    : ComponentMovementWatcher(&component), callback(callback) { }
    void componentMovedOrResized (bool, bool) override {}
    void componentPeerChanged () override {}
    void componentVisibilityChanged () override { callback(); }
private:
    std::function<void()> callback;
};

/**
 * An object holding a callback responding to a specific change in a value tree.
 */
struct ValueTreeChangedListener: public ValueTree::Listener
{
    ValueTreeChangedListener() = delete;
    ValueTreeChangedListener(ValueTree vt, const Identifier& id, std::function<void(const var&)> callback)
    : id(id), valueTree(std::move(vt)), callback(callback)
    {
        valueTree.addListener(this);
    }
    ~ValueTreeChangedListener()
    {
        valueTree.removeListener(this);
    }
    void valueTreePropertyChanged (ValueTree& vt, const Identifier &property) override
    {
        if (property == id && valueTree == vt)
            callback(vt.getProperty(property));
    }
    // Unused
    void valueTreeChildAdded (ValueTree&, ValueTree&) override {}
    void valueTreeChildRemoved (ValueTree&, ValueTree&, int) override {}
    void valueTreeChildOrderChanged (ValueTree&, int, int) override {};
    void valueTreeParentChanged (ValueTree&) override {}
    void valueTreeRedirected (ValueTree&) override {}
private:
    const Identifier& id;
    ValueTree valueTree;
    std::function<void(const var&)> callback;
};

template<class ValueType, class T>
bool withinRange(Range<ValueType> r, T value)
{
    return r.getStart() <= value &&  value <= r.getEnd();
}

template<class ValueType, class T>
T loopInRange(Range<ValueType> r, T value)
{
    if (r.getLength() == 0)
        return 0;

    return ((value - r.getStart()) % r.getLength()) + r.getStart();
}

struct TimestampComparison
{
    bool operator() (const MidiMessage& lhs, const MidiMessage& rhs) const
    {
        return lhs.getTimeStamp() < rhs.getTimeStamp();
    }
};

inline void copyBuffers(const AudioBuffer<float>& input, int startInput, AudioBuffer<float>& output, int startOutput, int numSamples)
{
    int numChannels = std::min(input.getNumChannels(), output.getNumChannels());
    for (int chanIdx = 0; chanIdx < numChannels; ++chanIdx)
        output.copyFrom(chanIdx, startOutput, input, chanIdx, startInput, numSamples);
}

template<class C, class T>
auto contains(const C& v, const T& x)
-> decltype(end(v), true)
{
    return end(v) != std::find(begin(v), end(v), x);
}