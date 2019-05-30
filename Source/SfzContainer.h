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

template<class ValueType>
class SfzContainer
{
public:
    SfzContainer(const ValueType& defaultValue)
    : defaultValue(defaultValue) { }

    const ValueType& getWithDefault(int index) const
    {
        auto it = container.find(index);
        if (it == end(container))
        {
            return defaultValue;
        }
        else
        {
            return it->second;
        }
    }

    bool contains(int index) const
    {
        return container.find(index) != end(container);
    }

    const ValueType& get(int index) const
    {
        return container.at(index);
    }
    
    ValueType& operator[](const int& key)
    {
        if (!contains(key))
            container.emplace(key, defaultValue);
        return container.operator[](key);
    }
private:
    const ValueType defaultValue;
    std::map<int, ValueType> container;
};