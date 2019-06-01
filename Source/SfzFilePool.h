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

struct SfzFile
{
    SfzFile() = default;
    SfzFile(std::shared_ptr<AudioBuffer<float>> preloadedBuffer, const File& file)
    : preloadedBuffer(preloadedBuffer), file(file) {}
    std::shared_ptr<AudioBuffer<float>> preloadedBuffer;
    File file;
};

class SfzReader
{
public: virtual int read(AudioBuffer<float> buffer, int startSample, int numSamples) = 0;
};

class SfzSineReader: public SfzReader
{
public:
    int read(AudioBuffer<float> buffer, int startSample, int numSamples) override
    {

    }
private:
};

class SfzFileReader: public SfzReader
{
public:
    SfzFileReader(std::shared_ptr<AudioBuffer<float>> preloadedBuffer)
    : preloadedBuffer(preloadedBuffer)
    {

    }
    int read(AudioBuffer<float> buffer, int startSample, int numSamples) override
    {
        
    }
private:
    std::shared_ptr<AudioBuffer<float>> preloadedBuffer;
};

class SfzFilePool
{
public:
    SfzFilePool()
    {

    }
    SfzFile preloadFile(const File& sampleFile, int offset = 0, int numSamples = config::preloadSize)
    {
        if (sampleFile.existsAsFile())
        {
            jassert(numSamples > 0);

            // Check if present
            auto existingFile = std::find_if(openFiles.begin(), openFiles.end(), [&sampleFile] (SfzFile& sfzFile) { return sfzFile.file == sampleFile; });
            if (existingFile != openFiles.end())
            {
                return *existingFile;
            }
            else
            {
                std::shared_ptr<AudioFormatReader> reader;
                if (wavFormat.canHandleFile(sampleFile))
                {
                    auto mappedReader = std::unique_ptr<MemoryMappedAudioFormatReader>(wavFormat.createMemoryMappedReader(sampleFile));
                    if (mappedReader == nullptr)
                    {
                        DBG("Error creating reader for " << sampleFile.getFullPathName());
                        return {};
                    }
                    mappedReader->mapEntireFile();
                    reader = std::shared_ptr<AudioFormatReader>(mappedReader.release());
                    
                }
                else if (oggFormat.canHandleFile(sampleFile))
                {
                    reader = std::shared_ptr<AudioFormatReader>(oggFormat.createReaderFor(sampleFile.createInputStream(), false));
                    if (reader == nullptr)
                    {
                        DBG("Error creating reader for " << sampleFile.getFullPathName());
                        return {};
                    }
                }
                else if (flacFormat.canHandleFile(sampleFile))
                {
                    reader = std::shared_ptr<AudioFormatReader>(flacFormat.createReaderFor(sampleFile.createInputStream(), false));
                    if (reader == nullptr)
                    {
                        DBG("Error creating reader for " << sampleFile.getFullPathName());
                        return {};
                    }
                }                

                const int actualNumSamples = (int)jmin((int64)numSamples, reader->lengthInSamples - offset);
                auto preloadedData = std::make_shared<AudioBuffer<float>>(config::numChannels, actualNumSamples);
                preloadedData->clear();
                reader->read(preloadedData.get(), 0, actualNumSamples, offset, true, true);
                return openFiles.emplace_back(reader, preloadedData, sampleFile);
            }
        }

        return {};
    }

    void clear()
    {
        openFiles.clear();
    }
private:    
    FlacAudioFormat flacFormat;
    OggVorbisAudioFormat oggFormat;
    WavAudioFormat wavFormat;
    std::vector<SfzFile> openFiles;
};