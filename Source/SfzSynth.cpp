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

#include "SfzSynth.h"
#include <string>
#include <regex>

SfzSynth::SfzSynth()
{
	resetMidiState();
	initalizeVoices();
}

SfzSynth::~SfzSynth()
{

}

std::string removeComment(const std::string& line)
{
	return std::regex_replace(line, std::regex(R"(//.*$)"), "");
}

std::string removeEOL(const std::string& line)
{
	return std::regex_replace(line, std::regex(R"(\r)"), "");
}

bool testEmptyLine(const std::string& line)
{
	return std::regex_match(line, std::regex(R"(^\s*$)"));
}

void SfzSynth::initalizeVoices(int numVoices)
{
	for (int i = 0; i < config::numVoices; ++i)
	{
		auto & voice = voices.emplace_back(fileLoadingPool, filePool, ccState);
		voice.prepareToPlay(sampleRate, samplesPerBlock);
	}
}

std::string SfzSynth::parseInclude(const std::string& line)
{
	std::smatch includeMatch;
	if (std::regex_search(line, includeMatch, SfzRegexes::includes))
	{
		auto includedFilename = includeMatch[1];
		File newFile { rootDirectory.getChildFile(String(includedFilename)) };
		if (!newFile.exists())
		{
			DBG("File not found: " << includedFilename);
			return line;
		}

		// Check if file was included
		auto alreadyIncluded = std::find(includedFiles.begin(), includedFiles.end(), newFile);
		if (alreadyIncluded != includedFiles.end())
		{
			// File was already included: there's an include loop somewhere...
			DBG("File already included: " << includedFilename);
			return line;
		}

		return readSfzFile(newFile);
	}
	return line;
}

std::string SfzSynth::readSfzFile(const juce::File &file)
{
	std::string fullString;
	fullString.reserve(file.getSize());
	StringArray destLines;
	file.readLines(destLines);
	includedFiles.push_back(file);

	if (destLines.size() == 0)
		return {};

	for (auto& line: destLines)
	{
		auto currentLine = line.toStdString();
		currentLine = removeEOL(currentLine);
		currentLine = removeComment(currentLine);
		currentLine = parseInclude(currentLine);
		if (!testEmptyLine(currentLine))
		{
			fullString += currentLine;
			fullString += ' ';
		}
	}

	return fullString;
}

std::string SfzSynth::expandDefines(const std::string& str)
{
	auto defineIterator = std::sregex_iterator(str.begin(), str.end(), SfzRegexes::defines);
	auto outputString = str;
	const auto regexEnd = std::sregex_iterator();
	for (; defineIterator != regexEnd; ++defineIterator)
  	{
		auto fullMatch = defineIterator->str(0);
		auto variableName = defineIterator->str(1);
		auto variableValue = defineIterator->str(2);
		fullMatch = std::regex_replace(fullMatch, std::regex(R"(\$)"), R"(\$)");
		variableName = std::regex_replace(variableName, std::regex(R"(\$)"), R"(\$)");
		outputString = std::regex_replace(outputString, std::regex(fullMatch), "");
		outputString = std::regex_replace(outputString, std::regex(variableName), variableValue);
	}
	return outputString;
}

bool SfzSynth::loadSfzFile(const juce::File &file)
{
	clear();
	if (!file.existsAsFile())
		return false;
	
	rootDirectory = file.getParentDirectory();
	filePool.setRootDirectory(rootDirectory);
	auto fullString = readSfzFile(file);
	fullString = expandDefines(fullString);

	auto headerIterator = std::sregex_iterator(fullString.begin(), fullString.end(), SfzRegexes::headers);
	auto regexEnd = std::sregex_iterator();
	uint32_t maxGroup { 1 };
	std::optional<uint8_t> defaultSwitch {};

	std::vector<SfzOpcode> globalMembers;
	std::vector<SfzOpcode> masterMembers;
	std::vector<SfzOpcode> groupMembers;
	std::vector<SfzOpcode> regionMembers;
	std::vector<SfzOpcode> curveMembers;
	std::vector<SfzOpcode> effectMembers;
	bool regionStarted = false;
	bool hasGlobal = false;
	bool hasControl = false;
	
	auto buildRegion = [&, this]() {
		regions.emplace_back(rootDirectory, filePool);
		auto& region = regions.back(); // For some reason using auto& region up there does not work?!
		// Successively apply the opcodes alread read to the parameter structure
		for (auto& opcode: globalMembers)
			region.parseOpcode(opcode);
		for (auto& opcode: masterMembers)
			region.parseOpcode(opcode);
		for (auto& opcode: groupMembers)
			region.parseOpcode(opcode);
		for (auto& opcode: regionMembers)
			region.parseOpcode(opcode);
		regionMembers.clear();	
	};

	for (; headerIterator != regexEnd; ++headerIterator)
  	{
		const auto header = headerIterator->str(1);
		const auto members = headerIterator->str(2);        
		auto paramIterator = std::sregex_iterator (members.begin(), members.end(), SfzRegexes::members);

		// If we had a building region and we encounter a new header we have to build it
		if (regionStarted)
		{
			buildRegion();
			regionStarted = false;
		}

		// Header logic
		switch (hash(header))
		{
			case hash("global"):
				if (hasGlobal)
					// We shouldn't have multiple global headers in file
					jassertfalse;
				else
					hasGlobal = true;
				break;
			case hash("control"):
				if (hasControl)
					// We shouldn't have multiple control headers in file
					jassertfalse;
				else
					hasControl = true;
				break;
			case hash("master"):
				numMasters += 1;
				groupMembers.clear();
				masterMembers.clear();
				break;
			case hash("group"):
				numGroups += 1;
				groupMembers.clear();
				break;
			case hash("region"):
				regionStarted = true;
				break;
			// TODO: handle these
			case hash("curve"):
				DBG("Curve header not implemented");
				break;
			case hash("effect"):
				DBG("Effect header not implemented");
				break;
			default:
				DBG("unknown header: " << header);
		}

		// Store or handlemembers
		std::for_each(paramIterator, regexEnd, [&](const auto& it){
			const auto opcode = it[1];
			const auto value = it[2];
			// Store the members depending on the header
			switch (hash(header))
			{
				case hash("global"):
					if (opcode == "sw_default")
						setValueFromOpcode({ opcode, value }, defaultSwitch, SfzDefault::keyRange);
					else
						globalMembers.emplace_back( opcode, value );
					break;
				case hash("master"):
					masterMembers.emplace_back( opcode, value );
					break;
				case hash("group"):
					if (opcode == "group")
					{
						try
						{
							const auto groupNumber = std::stoll(value);
							if (maxGroup < groupNumber)
								maxGroup = static_cast<uint32_t>(groupNumber);
						}
						catch (const std::exception& e [[maybe_unused]]) {}
					}
					groupMembers.emplace_back( opcode, value );
					break;
				case hash("region"):
					regionMembers.emplace_back( opcode, value );
					break;
				case hash("control"):
					{
						SfzOpcode lastOpcode { opcode, value };
						switch (hash(lastOpcode.opcode))
						{
						case hash("set_cc"):
							if (lastOpcode.parameter && withinRange(SfzDefault::ccRange, *lastOpcode.parameter))
								setValueFromOpcode(lastOpcode, ccState[*lastOpcode.parameter], SfzDefault::ccRange);
							break;
						case hash("label_cc"):
							if (lastOpcode.parameter && withinRange(SfzDefault::ccRange, *lastOpcode.parameter))
								ccNames.emplace_back(*lastOpcode.parameter, lastOpcode.value);
							break;
						case hash("default_path"):
							filePool.setRootDirectory(File(lastOpcode.value));
							break;
						default:
							DBG("Unknown/unsupported opcode in <control> header: " << lastOpcode.opcode);
						}
					}
					break;
				case hash("curve"):
					curveMembers.emplace_back( opcode, value );
					break;          
				case hash("effect"):
					effectMembers.emplace_back( opcode, value );          
			}
		});
	}
	
	// Build the last region
	if (regionStarted)
	{
		buildRegion();
		regionStarted = false;
	}

	// Find the groups and the offgroups
	newGroups.reserve(maxGroup);

	// Sort the CC labels
	std::sort(begin(ccNames), end(ccNames), [](auto& lhs, auto& rhs) { return lhs.first < rhs.first; });

	for (auto& region: regions)
	{
		region.prepare();
		
		for (int ccIdx = 1; ccIdx < 128; ccIdx++)
		{
			region.updateSwitches(MidiMessage::controllerEvent((int)region.channelRange.getStart(), ccIdx, ccState[ccIdx]));
		}

		if (defaultSwitch)
		{
			region.updateSwitches(MidiMessage::noteOn(1, *defaultSwitch, 1.0f));
			region.updateSwitches(MidiMessage::noteOff(1, *defaultSwitch));
		}
	}
	return true;
}

StringArray SfzSynth::getUnknownOpcodes() const
{
	StringArray returnedArray;
	for (auto& region: regions)
	{
		for (auto& opcode: region.unknownOpcodes)
		{
			if (!returnedArray.contains(opcode.opcode))
				returnedArray.add(opcode.opcode);
		}
	}
	return returnedArray;
}
StringArray SfzSynth::getCCLabels() const
{
	StringArray returnedArray;
	for (auto& ccNamePair: ccNames)
	{
		String s;
		s << ccNamePair.first << ": " << ccNamePair.second;
		returnedArray.add(s);
	}
	return returnedArray;
}

const SfzRegion* SfzSynth::getRegionView(int num) const
{
	if (num >= getNumRegions())
		return {};

	auto regionIterator = regions.cbegin();
	for (int i = 0; i < num; i++)
		regionIterator++;
	return &*regionIterator;
}

void SfzSynth::clear()
{
	regions.clear();
	voices.clear();
	ccNames.clear();
	filePool.clear();
	initalizeVoices();
	resetMidiState();
}

void SfzSynth::resetMidiState()
{
	for (auto& cc: ccState)
		cc = 0;
}

void SfzSynth::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	this->sampleRate = sampleRate;
	this->samplesPerBlock = samplesPerBlock;
	for (auto& voice: voices)
		voice.prepareToPlay(sampleRate, samplesPerBlock);
	tempBuffer = AudioBuffer<float>(config::numChannels, samplesPerBlock);
}

void SfzSynth::updateMidiState(const MidiMessage& msg, int timestamp)
{
	if (msg.isController())
		ccState[msg.getControllerNumber()] = msg.getControllerValue();
}

void SfzSynth::renderNextBlock(AudioBuffer<float>& outputAudio, const MidiBuffer& inputMidi)
{
	MidiBuffer::Iterator it { inputMidi };
	MidiMessage msg;
	int timestamp;
	const auto numSamples = outputAudio.getNumSamples();
	while (it.getNextEvent(msg, timestamp))
	{
		// TODO: check this up
		// High resolution MIDI stuff...
		// if (msg.isController() && msg.getControllerNumber() == 88)
		// 	continue;

		if (msg.isController())
			ccState[msg.getControllerNumber()] = msg.getControllerValue();

		// DBG("Midi timestamp " << timestamp); 
		DBG("[Timestamp " << timestamp << " ] Midi event: " << msg.getDescription()); 
		// Process MIDI for the existing voices
		for (auto& voice: voices)
			voice.processMidi(msg, timestamp);

		// Update the region activation for the message
		for (auto& region: regions)
			region.updateSwitches(msg);

		// DEBUG TRAP
		// auto numActiveRegions = std::count_if(begin(regions), end(regions), [](auto& region){ return region.isSwitchedOn(); });
		// DBG("Active regions: " << numActiveRegions);

		checkRegionsForActivation(msg, timestamp);        
	}
	
	// Render all the voices
	for (auto& voice: voices)
	{
		voice.renderNextBlock(tempBuffer, 0, numSamples);
		for (int channelIdx = 0; channelIdx < outputAudio.getNumChannels(); ++channelIdx)
			outputAudio.addFrom(channelIdx, 0, tempBuffer, channelIdx, 0, numSamples);
	}
}

void SfzSynth::checkRegionsForActivation(const MidiMessage& msg, int timestamp)
{
	const auto randValue = Random::getSystemRandom().nextFloat();
	for (auto& region: regions)
	{
		if (region.appliesTo(msg, randValue))
		{
			auto freeVoice = std::find_if(voices.begin(), voices.end(), [](auto& voice) { return voice.isFree(); });
			if (freeVoice != end(voices))
			{
				DBG("Found an active region on note " << msg.getNoteNumber() << " with sample " << region.sample);
				freeVoice->startVoice(region, msg, timestamp);
				for (auto& voice: voices)
				{
					const auto triggeringMessage = voice.getTriggeringMessage();
					if (triggeringMessage 
						&& &voice != &*freeVoice 
						&& voice.checkOffGroup(region.group, timestamp) 
						&& !region.isRelease())
					{
						checkRegionsForActivation(MidiMessage::noteOff(triggeringMessage->getChannel(), triggeringMessage->getNoteNumber()), timestamp);
					}
				}
			}
		}
	}
}