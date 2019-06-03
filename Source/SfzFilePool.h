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

// class SfzReader
// {
// public: virtual int read(AudioBuffer<float> buffer, int startSample, int numSamples) = 0;
// };

// class SfzSilenceReader: public SfzReader
// {
// public:
//     int read(AudioBuffer<float> buffer, int startSample, int numSamples) override
//     {
//         buffer.clear(startSample, numSamples);
//         return numSamples;
//     }
// };

// class SfzSineReader: public SfzReader
// {
// public:
//     SfzSineReader(uint8_t pitch, double sampleRate)
//     : pitch(pitch), sampleRate(sampleRate)
//     {

//     }
//     int read(AudioBuffer<float> buffer, int startSample, int numSamples) override
//     {
//         const auto frequency = MathConstants<float>::twoPi * MidiMessage::getMidiNoteInHertz(pitch);
//         const auto endSample = startSample + numSamples;
//         for(int sampleIdx = startSample; sampleIdx < endSample; sampleIdx++)
//         {
//             const float sampleValue = static_cast<float>(std::sin(frequency * sampleIdx / sampleRate));
//             for (int chanIdx = 0; chanIdx < config::numChannels; chanIdx++)
//             {
//                 buffer.setSample(chanIdx, sampleIdx, sampleValue);
//             }
//         }
//         return numSamples;
//     }
// private:
//     uint8_t pitch { SfzDefault::pitchKeycenter };
//     double sampleRate { config::defaultSampleRate };
// };

// class SfzFileReader: public SfzReader
// {
// public:
//     SfzFileReader(AudioFormatManager& formatManager, const File& sampleFile, std::shared_ptr<AudioBuffer<float>> preloadedBuffer)
//     : preloadedBuffer(preloadedBuffer)
//     , formatManager(formatManager)
//     , file(file)
//     {

//     }
//     int read(AudioBuffer<float> buffer, int startSample, int numSamples) override
//     {
        
//     }
// private:
//     AudioFormatManager& formatManager;
//     File file;
//     std::shared_ptr<AudioBuffer<float>> preloadedBuffer;
// };

class SfzFilePool
{
public:
    SfzFilePool(const File& rootDirectory)
    : rootDirectory(rootDirectory)
    {
        audioFormatManager.registerBasicFormats();
        // audioFormatManager.registerFormat(new FlacAudioFormat(), false);
        // audioFormatManager.registerFormat(new OggVorbisAudioFormat(), false);
    }

    void setRootDirectory(const File& rootDirectory)
    {
        if (rootDirectory.isDirectory())
            this->rootDirectory = rootDirectory;
    }

    // TODO: The sfz synth will update the regions at the end and preload the files, checking the offsets w.r.t. the preload size.
    // In the voice, update the file reading to prioritize the preloaded data if available.
    void preload(const String& sampleName, int offset = 0, int numSamples = config::preloadSize)
    {
        // TODO: if numSamples is negative preload everthing?
        // Can't really preload 0 samples now?
        jassert(numSamples > 0);
        
        if (sampleName.startsWith("*"))
            return;

        auto reader = createReaderFor(sampleName);
        if (reader == nullptr)
        {
            DBG("Error creating reader for " << sampleName);
            return;
        }
        const int actualNumSamples = (int) jmin( (int64)numSamples + offset, reader->lengthInSamples);
        auto alreadyPreloaded = preloadedData.find(sampleName);
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