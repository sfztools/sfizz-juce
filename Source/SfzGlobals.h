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
#include "SfzDefaults.h"
#include <regex>
#include <array>

using CCValueArray = std::array<uint8_t, 128>;
using CCValuePair = std::pair<uint8_t, float> ;
using CCNamePair = std::pair<uint8_t, String>;

namespace config
{
    inline constexpr int bufferSize { 8192 };
    inline constexpr double defaultSampleRate { 48000 };
    inline constexpr int defaultSamplesPerBlock { 1024 };
    inline constexpr int preloadSize { bufferSize };
    inline constexpr int numChannels { 2 };
    inline constexpr int numVoices { 128 };
    inline constexpr int maxGroups { 32 };
    inline constexpr int numLoadingThreads { 4 };
    inline constexpr int midiFeedbackCapacity { numVoices };
    inline constexpr int centPerSemitone { 100 };
    inline constexpr int loopCrossfadeLength { 64 };
    inline constexpr float virtuallyZero { 0.00005f };
    inline constexpr double fastReleaseDuration { 0.01 };
    inline constexpr int leftChan { 0 };
    inline constexpr int rightChan { 0 };
}

namespace SfzRegexes
{
    inline static std::regex includes { R"V(#include\s*"(.*?)".*$)V" };
    inline static std::regex defines { R"(#define\s*(\$[a-zA-Z0-9]+)\s+([a-zA-Z0-9]+)(?=\s|$))" };
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

inline unsigned int hash(const std::string& s, unsigned int h = Fnv1aBasis) { return hash(s.c_str(), h); }

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

// Constant power pan rule
inline void applyPanToSample(float pan, float& left, float& right)
{
    const float normalizedPan = pan / SfzDefault::panRange.getEnd();
    const float circlePan = MathConstants<float>::pi * (normalizedPan + 1) / 4;
    if (pan < 0) 
        std::swap(left, right);
    left *= dsp::FastMathApproximations::cos(circlePan);
    right *= dsp::FastMathApproximations::sin(circlePan);
}

inline void applyWidthAndPositionToSample(float width, float position, float& left, float& right)
{
    const float normalizedWidth = width / SfzDefault::widthRange.getEnd();
    const float circleWidth = MathConstants<float>::pi * (normalizedWidth + 1) / 4;
    const float normalizedPosition = position / SfzDefault::positionRange.getEnd();
    const float circlePosition = MathConstants<float>::pi * (normalizedPosition + 1) / 4;
    float mid = (left + right) / MathConstants<float>::sqrt2;
    float side = (left - right) / MathConstants<float>::sqrt2;
    mid *= dsp::FastMathApproximations::cos(circleWidth);
    side *= dsp::FastMathApproximations::sin(circleWidth);
    left = (mid + side) * dsp::FastMathApproximations::cos(circlePosition) / MathConstants<float>::sqrt2;
    right = (mid - side) * dsp::FastMathApproximations::sin(circlePosition) / MathConstants<float>::sqrt2;
}
