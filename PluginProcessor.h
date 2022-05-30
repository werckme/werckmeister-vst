#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <thread>
#include <list>
#include "PluginStateData.h"
#include "ILogger.h"


class AudioPluginAudioProcessor : public juce::AudioProcessor, public ILogger
{
public:
	typedef std::list<std::string> LogCache;
	AudioPluginAudioProcessor();
	~AudioPluginAudioProcessor() override;
	void prepareToPlay(double sampleRate, int samplesPerBlock) override;
	void releaseResources() override;
	bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
	void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
	using AudioProcessor::processBlock;
	juce::AudioProcessorEditor* createEditor() override;
	bool hasEditor() const override;
	const juce::String getName() const override;
	bool acceptsMidi() const override;
	bool producesMidi() const override;
	bool isMidiEffect() const override;
	double getTailLengthSeconds() const override;
	int getNumPrograms() override;
	int getCurrentProgram() override;
	void setCurrentProgram(int index) override;
	const juce::String getProgramName(int index) override;
	void changeProgramName(int index, const juce::String& newName) override;
	void getStateInformation(juce::MemoryBlock& destData) override;
	void setStateInformation(const void* data, int sizeInBytes) override;
	void compile(const juce::String& path);
	void reCompile();
	void log(ILogger::LogFunction) override;
	void info(ILogger::LogFunction f) override { log(f); }
	void warn(ILogger::LogFunction f) override { log(f); }
	void error(ILogger::LogFunction f) override { log(f); }
	const LogCache& getLogCache() const { return logCache; }
private:
	PluginStateData pluginStateData;
	typedef std::mutex Mutex;
	struct NoteOffStackItem 
	{
		const juce::MidiMessage* noteOff;
		int offsetInSamples = 0;
	};
	typedef std::list<NoteOffStackItem> NoteOffStack;
	NoteOffStack noteOffStack;
	Mutex processMutex;
	typedef juce::MidiMessageSequence::MidiEventHolder const* const* MidiEventIterator;
	void sendAllNoteOff(juce::MidiBuffer&);
	typedef std::vector<MidiEventIterator> IteratorTrackMap;
	IteratorTrackMap _iteratorTrackMap;
	juce::MidiFile _midiFile;
	bool _lastIsPlayingState = false;
	void processNoteOffStack(juce::MidiBuffer& midiMessages);
	LogCache logCache;
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioPluginAudioProcessor)
};
