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
#include "PluginProcessor.h"
#include "JuceHelpers.h"

class SfzFileChooser: public Component, public FileBrowserListener 
{
public:
    SfzFileChooser(SfzpluginAudioProcessor& p)
    : fileBrowser(FileBrowserComponent(
        FileBrowserComponent::saveMode | FileBrowserComponent::canSelectFiles | FileBrowserComponent::canSelectMultipleItems,
        File::getSpecialLocation(File::SpecialLocationType::userDocumentsDirectory),
        nullptr,
        nullptr))
    , processor(p)
    {
        addAndMakeVisible(fileBrowser);
        fileBrowser.addListener(this);
        if (auto* innerFileListComponent = dynamic_cast<FileListComponent*>(fileBrowser.getDisplayComponent()))
        {
            innerFileListComponent->getViewport()->setScrollOnDragEnabled(true);
            // This allows multiple selection on touch devices
            // innerFileListComponent->setClickingTogglesRowSelection(false);
        }

        addAndMakeVisible(okButton);
        okButton.setButtonText("OK");
        okButton.onClick = [&](){ addSelectedFiles(); };
        addAndMakeVisible(cancelButton);
        cancelButton.setButtonText("Cancel");
        cancelButton.onClick = [&](){
            fileBrowser.deselectAllFiles();
            this->setVisible(false);
        };
    }

    void paint(Graphics & g)
    {
        g.fillAll (getLookAndFeel().findColour (ResizableWindow::backgroundColourId));
        auto r = getLocalBounds();
        auto rightColumn = r.removeFromRight(80).reduced(5);
        okButton.setBounds(rightColumn.removeFromTop(30));
        cancelButton.setBounds(rightColumn.removeFromTop(30));
        fileBrowser.setBounds(r);
    }

    // FileBrowserListener callbacks
    void selectionChanged() override {}
    void fileClicked (const File&, const MouseEvent&) override {}
    void fileDoubleClicked (const File&) override { addSelectedFiles(); }
    void browserRootChanged (const File&) override {}
private:
    void addSelectedFiles()
    {
        if (fileBrowser.getNumSelectedFiles() > 0)
        {
            processor.loadSfz(fileBrowser.getSelectedFile(0));
        }
        fileBrowser.deselectAllFiles();
        this->setVisible(false);
    }

    FileBrowserComponent fileBrowser;
    TextButton okButton;
    TextButton cancelButton;
    SfzpluginAudioProcessor& processor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(SfzFileChooser)
};

//==============================================================================
/**
*/
class SfzpluginAudioProcessorEditor  : public AudioProcessorEditor, public Timer
{
public:
    SfzpluginAudioProcessorEditor (SfzpluginAudioProcessor&, MidiKeyboardState&, const SfzSynth&);
    ~SfzpluginAudioProcessorEditor();

    //==============================================================================
    void paint (Graphics&) override;
    void resized() override;

    void timerCallback() override
    {
        String s;
        s << "Active voices: " << processor.getNumActiveVoices();
        numVoices.setText(s, dontSendNotification);
    }

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    SfzpluginAudioProcessor& processor;
    MidiKeyboardComponent keyboardComponent;
    const SfzSynth& synth;
    TextButton openButton;
    Label numVoices;
    SfzFileChooser sfzChooser;
    TextEditor textBox;
    SimpleVisibilityWatcher<SfzFileChooser> watcher { sfzChooser, [this](){ 
        if (!sfzChooser.isVisible())
        {
            String text;
            text << "Masters: " << processor.getNumMasters() << newLine;
            text << "Groups: " << processor.getNumGroups() << newLine;
            text << "Regions: " << processor.getNumRegions() << newLine;
            text << "Unknown opcodes: " << processor.getUnknownOpcodes().joinIntoString(", ") << newLine;
            text << "Included Files: " << newLine;
            for (const auto& included: synth.getIncludedFiles())
                text << "- " << included << newLine;  
            text << "Defines: " << newLine;
            for (const auto& define: synth.getDefines())
                text << "- " << define.first << ": " << define.second << newLine;  
            auto labels = processor.getCCLabels();
            text << "CC Labels: " << newLine;
            for (auto& label: labels)
                text << "- " << label << newLine;
            textBox.setText(text);
        }
    }};

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SfzpluginAudioProcessorEditor)
};
