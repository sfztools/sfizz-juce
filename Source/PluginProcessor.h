/*
    ==============================================================================

    This file was partly auto-generated by JUCE.

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
#include "SfzSynth.h"

//==============================================================================
/**
*/
class SfzpluginAudioProcessor  : public AudioProcessor
{
public:
    //==============================================================================
    SfzpluginAudioProcessor();
    ~SfzpluginAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
    #endif

    void processBlock (AudioBuffer<float>&, MidiBuffer&) override;

    //==============================================================================
    AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const String getProgramName (int index) override;
    void changeProgramName (int index, const String& newName) override;

    //==============================================================================
    void getStateInformation (MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;
    //==============================================================================

    void loadSfz(const File& sfzFile)
    {
        sfzSynth.clear();
        sfzSynth.loadSfzFile(sfzFile);
    }

    StringArray getRegionList() const
    {
        StringArray returnedList;
        for (int regionIdx = 0; regionIdx < sfzSynth.getNumRegions(); regionIdx++)
        {
            returnedList.add(sfzSynth.getRegionView(regionIdx)->stringDescription());
        }         
        
        return returnedList;
    }

    int getNumRegions() const { return sfzSynth.getNumRegions(); }
    int getNumGroups() const { return sfzSynth.getNumGroups(); }
    int getNumMasters() const { return sfzSynth.getNumMasters(); }
    StringArray getUnknownOpcodes() const { return sfzSynth.getUnknownOpcodes(); }
    StringArray getCCLabels() const { return sfzSynth.getCCLabels(); }
    
private:
    SfzSynth sfzSynth;
    double sampleRate { 48000 };
    MidiKeyboardState keyboardState;
    TimeSliceThread loadingThread { "Background loading thread" };
    AudioFormatManager formatManager;
    std::unique_ptr<BufferingAudioSource> audioSource;
    bool noteStarted { false };
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfzpluginAudioProcessor)
};
