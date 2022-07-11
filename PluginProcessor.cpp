#include <iostream>
#include <ctime>
#include "PluginEditor.h"
#include "PluginProcessor.h"
#include <algorithm>
#include "Preferences.h"
#include <boost/interprocess/sync/named_mutex.hpp> 

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
	initCompiler();
}

PluginProcessor::~PluginProcessor()
{
	fileWatcher.stopThread(3000);
	stopUdpSender();
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
		midiMessages.addEvent(it->noteOff, 0);
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
	processNoteOffStack(midiMessages);
	LOCK(processMutex);
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
	if (udpSender)
	{
		udpSender->currentTimeInQuarters = posInfo.timeInSeconds / currentSheetTempoInSecondsPerQuarterNote;
	}
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
						NoteOffStackItem noteOff = {noteOffMidiMessage, (int)noteOffSampleOffset};
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
		midiMessages.addEvent(it->noteOff, it->offsetInSamples);
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
	auto succeeded = compile(pluginStateData.sheetPath);
	if (!succeeded && pluginStateData.sheetPath.empty() == false) 
	{
		LOCK(processMutex);
		fileWatcher.setFileList({pluginStateData.sheetPath});
		int port = readPreferencesData().funkfeuerPort;
		startUdpSender(pluginStateData.sheetPath);
	}
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new PluginProcessor();
}

void PluginProcessor::reCompile()
{
	compile(pluginStateData.sheetPath);
}

void PluginProcessor::updateFileWatcher(const CompiledSheet& sheet)
{
	if (sheet.sources.empty()) 
	{
		return;
	}
	const auto& sources = sheet.sources;
	FileWatcher::FileList filesToWatch;
	filesToWatch.resize(sheet.sources.size());
	std::transform(sources.begin(), sources.end(), filesToWatch.begin(), [](const Source& source) { return source.path; });
	fileWatcher.setFileList(filesToWatch);
}

void PluginProcessor::initCompiler()
{
	Compiler compiler(*this);
	compiler.resetExecutablePath();
	auto version = compiler.getVersionStr();
	if (version.empty()) 
	{
		error(LogLambda(log << "The werckmeister compiler could not be found."));
		info(LogLambda(log << "Please install werckmeister on your system."));
		info(LogLambda(log << "https://werckme.github.io"));
		info(LogLambda(log << "If you installed werckmeister and still got this message, try to set the werckmeister installation path in the preferences."));
		compilerIsReady = false;
		return;
	}
	info(LogLambda(log << "Compiler: " << compiler.compilerExecutable() << " -- " << version));
	compilerIsReady = true;
}

bool PluginProcessor::compile(const juce::String& path)
{
	if (!compilerIsReady)
	{
		return false;
	}
	if (path.length() == 0)
	{
		return false;
	}
	if (!juce::File(path).exists())
	{
		return false;
	}
	Compiler compiler(*this);
	auto compilerResult = compiler.compile(path.toStdString());
	pluginStateData.sheetPath = path.toStdString();
	LOCK(processMutex);
	_midiFile.clear();
	_iteratorTrackMap.clear();
	mutedTracks.clear();
	if (!compilerResult)
	{
		return false;
	}
	compiledSheet = compilerResult;
	stopUdpSender();
	juce::MemoryInputStream fs(compiledSheet->midiData.data(), compiledSheet->midiData.size(), false);
	_midiFile.readFrom(fs);
	_midiFile.convertTimestampTicksToSeconds();
	auto numTracks = (size_t)_midiFile.getNumTracks();
	_iteratorTrackMap.resize(numTracks);
	trackNames.resize(numTracks);
	std::unordered_map<std::string, int> trackAppearances;
	trackAppearances.reserve((size_t)_midiFile.getNumTracks());
	for (size_t trackIdx = 0; trackIdx < numTracks; ++trackIdx)
	{
		auto track = _midiFile.getTrack((int)trackIdx);
		_iteratorTrackMap[trackIdx] = track->begin();
		findTrackName(trackIdx, trackAppearances);
		applyMutedTrackState(trackIdx);
	}
	currentSheetTempoInSecondsPerQuarterNote = getTempoInSecondsPerQuarterNote(_midiFile);
	auto editor = dynamic_cast<PluginEditor*>(getActiveEditor());
	if (editor != nullptr)
	{
		editor->tracksChanged();
	}
	updateFileWatcher(*compiledSheet);
	startUdpSender(path);
	return true;
}

void PluginProcessor::startUdpSender(const juce::String &path)
{
	int port = readPreferencesData().funkfeuerPort;
	auto pluginHost = juce::PluginHostType();
	udpSender = std::make_unique<funk::UdpSender>(this, path.toStdString(), port);
	udpSender->compiledSheet = compiledSheet;
	udpSender->hostDescription = pluginHost.getHostDescription();
	udpSender->startThread();
}

void PluginProcessor::stopUdpSender()
{
	if (udpSender)
	{
		udpSender->stopThread(1500);
		udpSender.reset();
	}
}

double PluginProcessor::getTempoInSecondsPerQuarterNote(const juce::MidiFile &midiFile)
{
	juce::MidiMessageSequence events;
	midiFile.findAllTempoEvents(events);
	if (events.getNumEvents() == 0)
	{
		return 0.5; // default is 120
	}
	// in werckmeister a piece has just one tempo event
	return (*events.begin())->message.getTempoSecondsPerQuarterNote();
}

void PluginProcessor::findTrackName(size_t trackIndex, std::unordered_map<std::string, int>& trackAppearances)
{
	std::string trackName;
	auto track = _midiFile.getTrack((int)trackIndex);
	for (auto eventIt = track->begin(); eventIt != track->end(); ++eventIt)
	{
		const auto& midiMessage = (*eventIt)->message;
		if (!midiMessage.isTrackNameEvent())
		{
			continue;
		}
		trackName = midiMessage.getTextFromTextMetaEvent().toStdString();
		break;
	}
	if (trackName.empty())
	{
		trackName = std::string("Unnamed Track");
	}
	auto trackNameAppearanceIt = trackAppearances.find(trackName);
	if (trackNameAppearanceIt == trackAppearances.end())
	{
		trackAppearances[trackName] = 1;
	}
	else
	{
		int trackCount = trackNameAppearanceIt->second + 1;
		trackAppearances[trackName] = trackCount;
		trackName = trackName + "(" + std::to_string(trackCount) + ")";
	}
	trackNames[trackIndex] = trackName;
}

void PluginProcessor::log(ILogger::LogFunction fLog)
{
	std::stringstream logStream;
	std::time_t t = std::time(nullptr);   // get time now
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
		pluginStateData.mutedTracks.insert(trackNames.at((size_t)trackIndex));
		return;
	}
	mutedTracks.erase(trackIndex);
	pluginStateData.mutedTracks.erase(trackNames.at((size_t)trackIndex));
}

bool PluginProcessor::isMuted(int trackIndex) const
{
	return mutedTracks.find(trackIndex) != mutedTracks.end();
}

void PluginProcessor::applyMutedTrackState(int trackIndex)
{
	auto trackName = trackNames.at((size_t)trackIndex);
	bool isMuted = pluginStateData.mutedTracks.find(trackName) != pluginStateData.mutedTracks.end();
	if (isMuted)
	{
		mutedTracks.insert(trackIndex);
	}
}