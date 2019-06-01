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
#include <regex>
#include <array>

using CCValueArray = std::array<uint8_t, 128>;
using CCValuePair = std::pair<uint8_t, float> ;
using CCNamePair = std::pair<uint8_t, String>;

namespace config
{
    inline constexpr int bufferSize { 4096 };
    inline constexpr double defaultSampleRate { 48000 };
    inline constexpr int defaultSamplesPerBlock { 1024 };
    inline constexpr int preloadSize { bufferSize * 2 };
    inline constexpr int numChannels { 2 };
    inline constexpr int numVoices { 128 };
    inline constexpr int maxGroups { 32 };
    inline constexpr int numLoadingThreads { 4 };
    inline constexpr int midiFeedbackCapacity { numVoices };
    inline constexpr int centPerSemitone { 100 };
    inline constexpr int loopCrossfadeLength { 64 };
    inline constexpr float virtuallyZero { 0.00005f };
    inline constexpr double fastReleaseDuration { 0.01 };
}

namespace SfzRegexes
{
    inline static std::regex includes { R"V(#include\s*"(.*)"\s*$)V" };
    inline static std::regex defines { R"(#define\s*(\$[a-zA-Z0-9]+)\s+([a-zA-Z0-9]+))" };
    inline static std::regex headers { R"(<(.*?)>(.*?)(?=<|$))" };
    inline static std::regex members { R"(([a-zA-Z0-9_]+)=([a-zA-Z0-9-_#.\/\s\\\(\),\*]+)(?![a-zA-Z0-9_]*=))" };
    inline static std::regex opcodeParameters{ R"(([a-zA-Z0-9\_]+?)([0-9]+)$)" };
}

inline constexpr unsigned int Fnv1aBasis = 0x811C9DC5;
inline constexpr unsigned int Fnv1aPrime = 0x01000193;
inline constexpr unsigned int hash(const char *s, unsigned int h = Fnv1aBasis)
{
    return !*s ? h : hash(s + 1, static_cast<unsigned int>((h ^ *s) * static_cast<unsigned long long>(Fnv1aPrime)));
}

template<class T>
inline constexpr double centsFactor(T cents, T centsPerOctave = 1200) { return std::pow(2, static_cast<double>(cents) / centsPerOctave); }

template<class T>
inline constexpr float normalizeCC(T ccValue)
{
    static_assert(std::is_integral<T>::value);
    return static_cast<float>(std::min(std::max(ccValue, static_cast<T>(0)), static_cast<T>(127))) / 127.0f;
}

template<class T>
inline constexpr float normalizePercents(T percentValue)
{
    return std::min(std::max(static_cast<float>(percentValue), 0.0f), 100.0f) / 100.0f;
}