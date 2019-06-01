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
#include "JuceHelpers.h"
#include "SfzGlobals.h"
#include "SfzRegion.h"
#include "SfzVoice.h"
#include <algorithm>
#include "SfzFilePool.h"

class SfzSynth
{
public:
    SfzSynth();
    ~SfzSynth();
    bool loadSfzFile(const File& file);
    void initalizeVoices(int numVoices = config::numVoices);
    void clear();

    void prepareToPlay(double sampleRate, int samplesPerBlock);
    void updateMidiState(const MidiMessage& msg, int timestamp);
    void renderNextBlock(AudioBuffer<float>& outputAudio, const MidiBuffer& inputMidi);
    
    int getNumRegions() const { return static_cast<int>(regions.size()); }
    int getNumGroups() const { return numGroups; }
    int getNumMasters() const { return numMasters; }
    StringArray getUnknownOpcodes() const;
    StringArray getCCLabels() const;
    const SfzRegion* getRegionView(int num) const;
private:
    File rootDirectory { File::getCurrentWorkingDirectory() };
    AudioFormatManager afManager;
    AudioBuffer<float> tempBuffer;
    int numGroups { 0 };
    int numMasters { 0 };
    ThreadPool fileLoadingPool { config::numLoadingThreads };
    std::string parseInclude(const std::string& line);
    std::string readSfzFile(const juce::File &file);
    std::string expandDefines(const std::string& str);
    SfzFilePool openFiles;
    double sampleRate { config::defaultSampleRate };
    int samplesPerBlock { config::bufferSize };
    // TODO: transform to list
    std::vector<SfzRegion> regions;
    std::list<SfzVoice> voices;
    std::vector<File> includedFiles;
    std::vector<int> newGroups;
    CCValueArray ccState;
    std::vector<CCNamePair> ccNames;

    void resetMidiState();
    void checkRegionsForActivation(const MidiMessage& msg, int timestamp);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfzSynth);
};