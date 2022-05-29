#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <iostream>
#include "Compiler.h"

#define LOCK(mutex) std::lock_guard<Mutex> guard(mutex)

//==============================================================================
AudioPluginAudioProcessor::AudioPluginAudioProcessor()
	: AudioProcessor(BusesProperties()
#if ! JucePlugin_IsMidiEffect
#if ! JucePlugin_IsSynth
		.withInput("Input", juce::AudioChannelSet::stereo(), true)
#endif
		.withOutput("Output", juce::AudioChannelSet::stereo(), true)
#endif
	)
{
}

AudioPluginAudioProcessor::~AudioPluginAudioProcessor()
{
}

//==============================================================================
const juce::String AudioPluginAudioProcessor::getName() const
{
	return JucePlugin_Name;
}

bool AudioPluginAudioProcessor::acceptsMidi() const
{
#if JucePlugin_WantsMidiInput
	return true;
#else
	return false;
#endif
}

bool AudioPluginAudioProcessor::producesMidi() const
{
#if JucePlugin_ProducesMidiOutput
	return true;
#else
	return false;
#endif
}

bool AudioPluginAudioProcessor::isMidiEffect() const
{
#if JucePlugin_IsMidiEffect
	return true;
#else
	return false;
#endif
}

double AudioPluginAudioProcessor::getTailLengthSeconds() const
{
	return 0.0;
}

int AudioPluginAudioProcessor::getNumPrograms()
{
	return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
				// so this should be at least 1, even if you're not really implementing programs.
}

int AudioPluginAudioProcessor::getCurrentProgram()
{
	return 0;
}

void AudioPluginAudioProcessor::setCurrentProgram(int index)
{
	juce::ignoreUnused(index);
}

const juce::String AudioPluginAudioProcessor::getProgramName(int index)
{
	juce::ignoreUnused(index);
	return {};
}

void AudioPluginAudioProcessor::changeProgramName(int index, const juce::String& newName)
{
	juce::ignoreUnused(index, newName);
}

//==============================================================================
void AudioPluginAudioProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
	juce::ignoreUnused(sampleRate, samplesPerBlock);
}

void AudioPluginAudioProcessor::releaseResources()
{
	// When playback stops, you can use this as an opportunity to free up any
	// spare memory, etc.
}

bool AudioPluginAudioProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
#if JucePlugin_IsMidiEffect
	juce::ignoreUnused(layouts);
	return true;
#else
	// This is the place where you check if the layout is supported.
	// In this template code we only support mono or stereo.
	// Some plugin hosts, such as certain GarageBand versions, will only
	// load plugins that support stereo bus layouts.
	if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
		&& layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
		return false;

	// This checks if the input layout matches the output layout
#if ! JucePlugin_IsSynth
	if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
		return false;
#endif

	return true;
#endif
}

void AudioPluginAudioProcessor::sendAllNoteOff(juce::MidiBuffer& midiMessages)
{
	for (int ch = 1; ch <= 16; ++ch)
	{
		midiMessages.addEvent(juce::MidiMessage::allNotesOff(ch), 0);
	}
	noteOffStack.clear();
}

void AudioPluginAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer,
	juce::MidiBuffer& midiMessages)
{
	juce::ScopedNoDenormals noDenormals;
	auto totalNumOutputChannels = getTotalNumOutputChannels();
	for (auto i = 0; i < totalNumOutputChannels; ++i)
	{
		buffer.clear(i, 0, buffer.getNumSamples());
	}
	LOCK(mutex);
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
		auto track = _midiFile.getTrack(trackIdx);
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
						midiMessages.addEvent(noteOffMidiMessage, noteOffSampleOffset);
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

void AudioPluginAudioProcessor::processNoteOffStack(juce::MidiBuffer& midiMessages)
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

bool AudioPluginAudioProcessor::hasEditor() const
{
	return true;
}

juce::AudioProcessorEditor* AudioPluginAudioProcessor::createEditor()
{
	return new AudioPluginAudioProcessorEditor(*this);
}

void AudioPluginAudioProcessor::getStateInformation(juce::MemoryBlock& destData)
{
	// You should use this method to store your parameters in the memory block.
	// You could do that either as raw data, or use the XML or ValueTree classes
	// as intermediaries to make it easy to save and load complex data.
	juce::ignoreUnused(destData);
}

void AudioPluginAudioProcessor::setStateInformation(const void* data, int sizeInBytes)
{
	// You should use this method to restore your parameters from this memory block,
	// whose contents will have been created by the getStateInformation() call.
	juce::ignoreUnused(data, sizeInBytes);
}

juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
	return new AudioPluginAudioProcessor();
}

void AudioPluginAudioProcessor::compile(const juce::String& path)
{
	Compiler compiler;
	auto version = compiler.getVersionStr();
	auto compileResult = compiler.compile(path.toStdString());
	LOCK(mutex);
	_midiFile.clear();
	_iteratorTrackMap.clear();
	juce::MemoryInputStream fs(compileResult.midiData.data(), compileResult.midiData.size(), false);
	_midiFile.readFrom(fs);
	_midiFile.convertTimestampTicksToSeconds();
	auto numTracks = (size_t)_midiFile.getNumTracks();
	_iteratorTrackMap.resize(numTracks);
	for (size_t trackIdx = 0; trackIdx < numTracks; ++trackIdx)
	{
		auto track = _midiFile.getTrack(trackIdx);
		_iteratorTrackMap[trackIdx] = track->begin();
	}
}