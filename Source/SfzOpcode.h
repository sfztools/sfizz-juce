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
#include "JuceHelpers.h"
#include <regex>
#include <string>
#include <optional>
#include <string_view>

struct SfzOpcode
{
    SfzOpcode() = delete;
    SfzOpcode(std::string_view inputOpcode, std::string_view inputValue)
    :opcode(inputOpcode), value(inputValue)
    {
        if (const auto lastCharIndex = inputOpcode.find_last_not_of("1234567890"); lastCharIndex != inputOpcode.npos)
        {
            try
            {
                const auto firstNumIndex = lastCharIndex + 1;
                const auto numberLength = inputOpcode.size() - firstNumIndex;
                std::string parameterNum(inputOpcode.substr(firstNumIndex, numberLength));
                parameter = std::stoi(parameterNum);
                opcode = std::string_view(inputOpcode.data(), firstNumIndex);
            }
            catch (const std::exception& e [[maybe_unused]])
            {
                parameter = {};
            }
            // Sketchy support for charconv in current versions of XCode, GCC, etc...
            // TODO: It would be better than std::stoXX though, since we would not require a temporary string
            // to be created. One can hope that such a string is small enough..
            //
            // int returnValue { 0 };
            // auto [ptr, errorCode] = std::from_chars(lastCharPtr + 1, inputOpcode.data() + inputOpcode.size(), returnValue);

            // if (errorCode == std::errc())
            //     parameter = returnValue;
            // opcode = std::string_view(inputOpcode.data(), std::distance(inputOpcode.data(), lastCharPtr + 1)  );
        }
        trimView(value);
        trimView(opcode);
    }

    std::string_view opcode{};
    std::string_view value{};	
    // This is to handle the integer parameter of some opcodes
    std::optional<int> parameter;
};

inline std::optional<uint8_t> readNoteValue(const std::string_view&value)
{
    switch(hash(value))
    {
        case hash("c-1"):   case hash("C-1"):   return 0;
        case hash("c#-1"):  case hash("C#-1"):  return 1;
        case hash("d-1"):   case hash("D-1"):   return 2;
        case hash("d#-1"):  case hash("D#-1"):  return 3;
        case hash("e-1"):   case hash("E-1"):   return 4;
        case hash("f-1"):   case hash("F-1"):   return 5;
        case hash("f#-1"):  case hash("F#-1"):  return 6;
        case hash("g-1"):   case hash("G-1"):   return 7;
        case hash("g#-1"):  case hash("G#-1"):  return 8;
        case hash("a-1"):   case hash("A-1"):   return 9;
        case hash("a#-1"):  case hash("A#-1"):  return 10;
        case hash("b-1"):   case hash("B-1"):   return 11;

        case hash("c0"):    case hash("C0"):    return 12;
        case hash("c#0"):   case hash("C#0"):   return 13;
        case hash("d0"):    case hash("D0"):    return 14;
        case hash("d#0"):   case hash("D#0"):   return 15;
        case hash("e0"):    case hash("E0"):    return 16;
        case hash("f0"):    case hash("F0"):    return 17;
        case hash("f#0"):   case hash("F#0"):   return 18;
        case hash("g0"):    case hash("G0"):    return 19;
        case hash("g#0"):   case hash("G#0"):   return 20;
        case hash("a0"):    case hash("A0"):    return 21;
        case hash("a#0"):   case hash("A#0"):   return 22;
        case hash("b0"):    case hash("B0"):    return 23;

        case hash("c1"):    case hash("C1"):    return 24;
        case hash("c#1"):   case hash("C#1"):   return 25;
        case hash("d1"):    case hash("D1"):    return 26;
        case hash("d#1"):   case hash("D#1"):   return 27;
        case hash("e1"):    case hash("E1"):    return 28;
        case hash("f1"):    case hash("F1"):    return 29;
        case hash("f#1"):   case hash("F#1"):   return 30;
        case hash("g1"):    case hash("G1"):    return 31;
        case hash("g#1"):   case hash("G#1"):   return 32;
        case hash("a1"):    case hash("A1"):    return 33;
        case hash("a#1"):   case hash("A#1"):   return 34;
        case hash("b1"):    case hash("B1"):    return 35;

        case hash("c2"):    case hash("C2"):    return 36;
        case hash("c#2"):   case hash("C#2"):   return 37;
        case hash("d2"):    case hash("D2"):    return 38;
        case hash("d#2"):   case hash("D#2"):   return 39;
        case hash("e2"):    case hash("E2"):    return 40;
        case hash("f2"):    case hash("F2"):    return 41;
        case hash("f#2"):   case hash("F#2"):   return 42;
        case hash("g2"):    case hash("G2"):    return 43;
        case hash("g#2"):   case hash("G#2"):   return 44;
        case hash("a2"):    case hash("A2"):    return 45;
        case hash("a#2"):   case hash("A#2"):   return 46;
        case hash("b2"):    case hash("B2"):    return 47;

        case hash("c3"):    case hash("C3"):    return 48;
        case hash("c#3"):   case hash("C#3"):   return 49;
        case hash("d3"):    case hash("D3"):    return 50;
        case hash("d#3"):   case hash("D#3"):   return 51;
        case hash("e3"):    case hash("E3"):    return 52;
        case hash("f3"):    case hash("F3"):    return 53;
        case hash("f#3"):   case hash("F#3"):   return 54;
        case hash("g3"):    case hash("G3"):    return 55;
        case hash("g#3"):   case hash("G#3"):   return 56;
        case hash("a3"):    case hash("A3"):    return 57;
        case hash("a#3"):   case hash("A#3"):   return 58;
        case hash("b3"):    case hash("B3"):    return 59;

        case hash("c4"):    case hash("C4"):    return 60;
        case hash("c#4"):   case hash("C#4"):   return 61;
        case hash("d4"):    case hash("D4"):    return 62;
        case hash("d#4"):   case hash("D#4"):   return 63;
        case hash("e4"):    case hash("E4"):    return 64;
        case hash("f4"):    case hash("F4"):    return 65;
        case hash("f#4"):   case hash("F#4"):   return 66;
        case hash("g4"):    case hash("G4"):    return 67;
        case hash("g#4"):   case hash("G#4"):   return 68;
        case hash("a4"):    case hash("A4"):    return 69;
        case hash("a#4"):   case hash("A#4"):   return 70;
        case hash("b4"):    case hash("B4"):    return 71;

        case hash("c5"):    case hash("C5"):    return 72;
        case hash("c#5"):   case hash("C#5"):   return 73;
        case hash("d5"):    case hash("D5"):    return 74;
        case hash("d#5"):   case hash("D#5"):   return 75;
        case hash("e5"):    case hash("E5"):    return 76;
        case hash("f5"):    case hash("F5"):    return 77;
        case hash("f#5"):   case hash("F#5"):   return 78;
        case hash("g5"):    case hash("G5"):    return 79;
        case hash("g#5"):   case hash("G#5"):   return 80;
        case hash("a5"):    case hash("A5"):    return 81;
        case hash("a#5"):   case hash("A#5"):   return 82;
        case hash("b5"):    case hash("B5"):    return 83;

        case hash("c6"):    case hash("C6"):    return 84;
        case hash("c#6"):   case hash("C#6"):   return 85;
        case hash("d6"):    case hash("D6"):    return 86;
        case hash("d#6"):   case hash("D#6"):   return 87;
        case hash("e6"):    case hash("E6"):    return 88;
        case hash("f6"):    case hash("F6"):    return 89;
        case hash("f#6"):   case hash("F#6"):   return 90;
        case hash("g6"):    case hash("G6"):    return 91;
        case hash("g#6"):   case hash("G#6"):   return 92;
        case hash("a6"):    case hash("A6"):    return 93;
        case hash("a#6"):   case hash("A#6"):   return 94;
        case hash("b6"):    case hash("B6"):    return 95;

        case hash("c7"):    case hash("C7"):    return 96;
        case hash("c#7"):   case hash("C#7"):   return 97;
        case hash("d7"):    case hash("D7"):    return 98;
        case hash("d#7"):   case hash("D#7"):   return 99;
        case hash("e7"):    case hash("E7"):    return 100;
        case hash("f7"):    case hash("F7"):    return 101;
        case hash("f#7"):   case hash("F#7"):   return 102;
        case hash("g7"):    case hash("G7"):    return 103;
        case hash("g#7"):   case hash("G#7"):   return 104;
        case hash("a7"):    case hash("A7"):    return 105;
        case hash("a#7"):   case hash("A#7"):   return 106;
        case hash("b7"):    case hash("B7"):    return 107;

        case hash("c8"):    case hash("C8"):    return 108;
        case hash("c#8"):   case hash("C#8"):   return 109;
        case hash("d8"):    case hash("D8"):    return 110;
        case hash("d#8"):   case hash("D#8"):   return 111;
        case hash("e8"):    case hash("E8"):    return 112;
        case hash("f8"):    case hash("F8"):    return 113;
        case hash("f#8"):   case hash("F#8"):   return 114;
        case hash("g8"):    case hash("G8"):    return 115;
        case hash("g#8"):   case hash("G#8"):   return 116;
        case hash("a8"):    case hash("A8"):    return 117;
        case hash("a#8"):   case hash("A#8"):   return 118;
        case hash("b8"):    case hash("B8"):    return 119;

        case hash("c9"):    case hash("C9"):    return 120;
        case hash("c#9"):   case hash("C#9"):   return 121;
        case hash("d9"):    case hash("D9"):    return 122;
        case hash("d#9"):   case hash("D#9"):   return 123;
        case hash("e9"):    case hash("E9"):    return 124;
        case hash("f9"):    case hash("F9"):    return 125;
        case hash("f#9"):   case hash("F#9"):   return 126;
        case hash("g9"):    case hash("G9"):    return 127;
        default:                                return {};
    }
}

template<class ValueType>
inline std::optional<ValueType> readOpcode(std::string_view value, const Range<ValueType>& validRange)
{
    if constexpr(std::is_integral<ValueType>::value)
    {
        // Sketchy support for charconv in current versions of XCode, GCC, etc...
        // TODO: It would be better than std::stoXX though, since we would not require a temporary string
        // to be created. One can hope that such a string is small enough..
        //
        // int64_t returnedValue;
        // auto [ptr, errorCode] = std::from_chars(value.data(), value.data() + value.size(), returnedValue);
        // if (errorCode != std::errc())
        //     return {};

        // if (returnedValue > std::numeric_limits<ValueType>::max())
        //     returnedValue = std::numeric_limits<ValueType>::max();
        // if (returnedValue < std::numeric_limits<ValueType>::min())
        //     returnedValue = std::numeric_limits<ValueType>::min();

        // return validRange.clipValue(static_cast<ValueType>(returnedValue));
        try
        {
            std::string valueStr { value };
            auto returnedValue = std::stoll(valueStr);
            
            if (returnedValue > std::numeric_limits<ValueType>::max())
                returnedValue = std::numeric_limits<ValueType>::max();
            if (returnedValue < std::numeric_limits<ValueType>::min())
                returnedValue = std::numeric_limits<ValueType>::min();

            return validRange.clipValue(static_cast<ValueType>(returnedValue));
        }
        catch (const std::exception& e [[maybe_unused]])
        {
            return {};
        }
    }
    else
    {
        // GCC does not support this as of 8.3
        // float returnedValue;
        // auto [ptr, errorCode] = std::from_chars(value.data(), value.data() + value.size(), returnedValue);
        // if (errorCode != std::errc())
        //     return {};

        // return validRange.clipValue(returnedValue);
        try
        {
            std::string valueStr { value };
            return validRange.clipValue(std::stof(valueStr));
        }
        catch (const std::exception& e [[maybe_unused]])
        {
            return {};
        }
    }
}

template<class ValueType>
inline void setValueFromOpcode(const SfzOpcode& opcode, ValueType& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target = *value;
}

template<class ValueType>
inline void setValueFromOpcode(const SfzOpcode& opcode, std::optional<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target = *value;
}

template<class ValueType>
inline void setRangeEndFromOpcode(const SfzOpcode& opcode, Range<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target.setEnd(*value);
}

template<class ValueType>
inline void setRangeStartFromOpcode(const SfzOpcode& opcode, Range<ValueType>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (!value) // Try and read a note rather than a number
        value = readNoteValue(opcode.value);
    if (value)
        target.setStart(*value);
}

template<class ValueType>
inline void setCCPairFromOpcode(const SfzOpcode& opcode, std::optional<CCValuePair>& target, const Range<ValueType>& validRange)
{
    auto value = readOpcode(opcode.value, validRange);
    if (value && opcode.parameter && withinRange(SfzDefault::ccRange, *opcode.parameter))
        target = std::make_pair(*opcode.parameter, *value);
    else
        target = {};
}