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
#include <memory>
#include <map>

class SfzFilePool
{
public:
    SfzFilePool(const File& rootDirectory)
    : rootDirectory(rootDirectory)
    {
        audioFormatManager.registerBasicFormats();
    }

    void setRootDirectory(const File& directory)
    {
        if (directory.isDirectory())
            this->rootDirectory = directory;
    }
    
    void preload(const String& sampleName, int offset = 0, int numSamples = config::preloadSize)
    {        
        if (sampleName.startsWith("*"))
            return;

        auto reader = createReaderFor(sampleName);
        if (reader == nullptr)
        {
            DBG("Error creating reader for " << sampleName);
            return;
        }

        const int actualNumSamples = [offset, numSamples, &reader](){
            if (numSamples > 0) 
                return (int) jmin( (int64)numSamples + offset, reader->lengthInSamples);
            else
                return static_cast<int>(reader->lengthInSamples);
        }();

        const auto alreadyPreloaded = preloadedData.find(sampleName);
        if (alreadyPreloaded == end(preloadedData) || alreadyPreloaded->second->getNumSamples() < actualNumSamples)
        {
            preloadedData[sampleName] = std::make_shared<AudioBuffer<float>>(config::numChannels, actualNumSamples);
            preloadedData[sampleName]->clear();
            reader->read(preloadedData[sampleName].get(), 0, actualNumSamples, 0, true, true);
        }
    }

    std::unique_ptr<AudioFormatReader> createReaderFor(const String& sampleName)
    {
        File sampleFile { rootDirectory.getChildFile(sampleName) };
        if (!sampleFile.existsAsFile())
        {
            DBG("Can't find file " << sampleName );
            return {};
        }
        return std::unique_ptr<AudioFormatReader>(audioFormatManager.createReaderFor(sampleFile));
    }

    void clear()
    {
        preloadedData.clear();
    }

    std::shared_ptr<AudioBuffer<float>> getPreloadedData(const String& sampleName)
    {
        auto data = preloadedData.find(sampleName);
        if (data != end(preloadedData))
            return data->second;
        
        return {};
    }

private:
    File rootDirectory;
    AudioFormatManager audioFormatManager;
    std::map<String, std::shared_ptr<AudioBuffer<float>>> preloadedData;
};