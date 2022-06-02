#include <iostream>
#include <ctime>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <algorithm>

#define LOCK(mutex) std::lock_guard<Mutex> guard(mutex)

PluginProcessor::PluginProcessor()
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
{
	fileWatcher.startThread();
	fileWatcher.onFileChanged = std::bind(&PluginProcessor::reCompile, this);
}

PluginProcessor::~PluginProcessor()
{
	fileWatcher.stopThread(3000);
}

void PluginProcessor::releaseResources()
{
}

const juce::String PluginProcessor::getName() const
{
	return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool PluginProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool PluginProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double PluginProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int PluginProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int PluginProcessor::getCurrentProgram()
{
	return 0;
}

void PluginProcessor::setCurrentProgram(int)
{
}

const juce::String PluginProcessor::getProgramName(int)
{
	return "";
}

void PluginProcessor::changeProgramName(int, const juce::String&)
{
}

void PluginProcessor::prepareToPlay(double, int)
{
}


bool PluginProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}

void PluginProcessor::sendAllNoteOff(juce::MidiBuffer& midiMessages)
{
	for (NoteOffStack::iterator it = noteOffStack.begin(); it != noteOffStack.end(); ++it)
	{
		midiMessages.addEvent(*it->noteOff, 0);
	}
	noteOffStack.clear();
}

void PluginProcessor::processBlock(juce::AudioBuffer<float>& buffer,
	juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumOutputChannels = getTotalNumOutputChannels();
	for (auto i = 0; i < totalNumOutputChannels; ++i)
	{
		buffer.clear(i, 0, buffer.getNumSamples());
	}
	LOCK(processMutex);
	processNoteOffStack(midiMessages);
	if(_midiFile.getNumTracks() == 0)
	{
		return;
	}
	auto playHead_ = getPlayHead();
	if (!playHead_) {
		return;
	}
	juce::AudioPlayHead::CurrentPositionInfo posInfo = {0};
	playHead_->getCurrentPosition(posInfo);
	if (!posInfo.isPlaying && _lastIsPlayingState) 
	{
		_lastIsPlayingState = false;
		sendAllNoteOff(midiMessages);
	}
	if (!posInfo.isPlaying) {
		return;
	}
	_lastIsPlayingState = true;
	auto beginPosSeconds = posInfo.timeInSeconds;
	auto endPosSeconds = posInfo.timeInSeconds + ((double)getBlockSize() / getSampleRate());

	for (size_t trackIdx = 0; trackIdx < (size_t)_midiFile.getNumTracks(); ++trackIdx)
	{
		if (isMuted(trackIdx))
		{
			continue;
		}
		auto track = _midiFile.getTrack((int)trackIdx);
		if (track->getNumEvents() == 0) 
		{
			continue;
		}
		auto eventIt = _iteratorTrackMap[trackIdx];
		if (eventIt == track->end())  
		{
			eventIt = track->begin();
		}
		bool playHeadIsBeforeCurrentIterator = beginPosSeconds < (*eventIt)->message.getTimeStamp();
		if (playHeadIsBeforeCurrentIterator) 
		{
			eventIt = track->begin();
		}
		while (true) {
			if (eventIt == track->end())
			{
				break;
			}
			const auto &midiMessage = (*eventIt)->message;
			auto eventTimeStamp = midiMessage.getTimeStamp();
			if (eventTimeStamp > endPosSeconds)
			{
				break;
			}
			int sampleOffset = std::max<double>(0.0, (eventTimeStamp - beginPosSeconds) * getSampleRate());
			bool isSendMessage = !midiMessage.isNoteOff() && (eventTimeStamp >= beginPosSeconds && eventTimeStamp <= endPosSeconds);
			if (isSendMessage) 
			{
				midiMessages.addEvent(midiMessage, (int)sampleOffset);
				auto corrospondingNoteOff = (*eventIt)->noteOffObject;
				if (corrospondingNoteOff)
				{
					const auto &noteOffMidiMessage = corrospondingNoteOff->message;
					auto noteOffSampleOffset = noteOffMidiMessage.getTimeStamp() * getSampleRate();
					noteOffSampleOffset -= eventTimeStamp * getSampleRate();
					if (noteOffSampleOffset < getBlockSize()) 
					{
						midiMessages.addEvent(noteOffMidiMessage, (int)noteOffSampleOffset);
					}
					else
					{
						NoteOffStackItem noteOff = {&noteOffMidiMessage, (int)noteOffSampleOffset};
						noteOffStack.emplace_back(noteOff);
					}
				}
			}
			++eventIt;
		}
		_iteratorTrackMap[trackIdx] = eventIt;
	}
}

void PluginProcessor::processNoteOffStack(juce::MidiBuffer& midiMessages)
{
	int blockSize_ = getBlockSize();
	std::list<NoteOffStack::iterator> toRemove;
	for(NoteOffStack::iterator it = noteOffStack.begin(); it != noteOffStack.end(); ++it)
	{
		it->offsetInSamples -= blockSize_;
		if (it->offsetInSamples > blockSize_)
		{
			continue;
		}
		midiMessages.addEvent(*it->noteOff, it->offsetInSamples);
		toRemove.push_back(it);
	}

	for (auto it : toRemove)
	{
		noteOffStack.erase(it);
	}  
}

bool PluginProcessor::hasEditor() const
{
	return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
	return new PluginEditor(*this);
}

void PluginProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	writeStateData(pluginStateData, destData);
}

void PluginProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	pluginStateData = readStateData(data, sizeInBytes);
	if (!pluginStateData.isValid)
	{
		return;
	}
	compile(pluginStateData.sheetPath);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new PluginProcessor();
}

void PluginProcessor::reCompile()
{
	compile(pluginStateData.sheetPath);
}

void PluginProcessor::updateFileWatcher(const CompiledSheet& compiledSheet)
{
	if (compiledSheet.sources.empty()) 
	{
		return;
	}
	const auto& sources = compiledSheet.sources;
	FileWatcher::FileList filesToWatch;
	filesToWatch.resize(compiledSheet.sources.size());
	std::transform(sources.begin(), sources.end(), filesToWatch.begin(), [](const Source& source) { return source.path; });
	fileWatcher.setFileList(filesToWatch);
}

void PluginProcessor::compile(const juce::String& path)
{
	if (path.length() == 0)
	{
		return;
	}
	if (!juce::File(path).exists())
	{
		return;
	}
	Compiler compiler(*this);
	auto version = compiler.getVersionStr();
	auto compileResult = compiler.compile(path.toStdString());
	pluginStateData.sheetPath = path.toStdString();
	LOCK(processMutex);
	_midiFile.clear();
	_iteratorTrackMap.clear();
	mutedTracks.clear();
	juce::MemoryInputStream fs(compileResult.midiData.data(), compileResult.midiData.size(), false);
	_midiFile.readFrom(fs);
	_midiFile.convertTimestampTicksToSeconds();
	auto numTracks = (size_t)_midiFile.getNumTracks();
	_iteratorTrackMap.resize(numTracks);
	trackNames.resize(numTracks);
	for (size_t trackIdx = 0; trackIdx < numTracks; ++trackIdx)
	{
		auto track = _midiFile.getTrack((int)trackIdx);
		_iteratorTrackMap[trackIdx] = track->begin();
		for (auto eventIt = track->begin(); eventIt != track->end(); ++eventIt)
		{
			const auto& midiMessage = (*eventIt)->message;
			if(!midiMessage.isTrackNameEvent()) 
			{
				continue;
			}
			trackNames[trackIdx] = midiMessage.getTextFromTextMetaEvent().toStdString() + "(" + std::to_string(trackIdx) + ")";
			break;
		}
		if (trackNames[trackIdx].empty())
		{
			trackNames[trackIdx] = std::string("Unnamed Track(" + std::to_string(trackIdx) + ")");
		}
		applyMutedTrackState(trackIdx);
	}
	auto editor = dynamic_cast<PluginEditor*>(getActiveEditor());
	if (editor != nullptr)
	{
		editor->tracksChanged();
	}
	updateFileWatcher(compileResult);
}

void PluginProcessor::log(ILogger::LogFunction fLog)
{
	std::stringstream logStream;
	std::time_t t = std::time(0);   // get time now
	std::tm* now = std::localtime(&t);
	logStream.fill('0');
	logStream << "[" 
		<< std::setw(2) << now->tm_hour
		<< ":" 
		<< std::setw(2) << now->tm_min
		<< ":" 
		<< std::setw(2) << now->tm_min
		<< ":" 
		<< std::setw(2) << now->tm_sec
		<< "] ";
	fLog(logStream);
	
	auto editor = dynamic_cast<PluginEditor*>(getActiveEditor());
	if (editor == nullptr) 
	{
		logCache.push_back(logStream.str());
		return;
	}
	juce::String line(logStream.str());
	editor->writeLine(line);
}

void PluginProcessor::onTrackFilterChanged(int trackIndex, bool filterValue)
{
	if (!filterValue)
	{
		mutedTracks.insert(trackIndex);
		pluginStateData.mutedTracks.insert(trackNames.at(trackIndex));
		return;
	}
	mutedTracks.erase(trackIndex);
	pluginStateData.mutedTracks.erase(trackNames.at(trackIndex));
}

bool PluginProcessor::isMuted(int trackIndex) const
{
	return mutedTracks.find(trackIndex) != mutedTracks.end();
}

void PluginProcessor::applyMutedTrackState(int trackIndex)
{
	auto trackName = trackNames.at(trackIndex);
	bool isMuted = pluginStateData.mutedTracks.find(trackName) != pluginStateData.mutedTracks.end();
	if (isMuted)
	{
		mutedTracks.insert(trackIndex);
	}
}