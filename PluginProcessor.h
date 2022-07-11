#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <vector>
#include <unordered_set>
#include <thread>
#include <list>
#include "PluginStateData.h"
#include "ILogger.h"
#include "FileWatcher.hpp"
#include "Compiler.h"
#include "UdpSender.hpp"
#include <memory>

class PluginProcessor : public juce::AudioProcessor, public ILogger
{
public:
	typedef int TrackIndex;
	typedef std::unordered_set<TrackIndex> MutedTracks;
	typedef std::list<std::string> LogCache;
	typedef std::vector<std::string> TrackNames;
	PluginProcessor();
	~PluginProcessor() override;
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
	bool compile(const juce::String& path);
	void reCompile();
	void log(ILogger::LogFunction) override;
	void info(ILogger::LogFunction f) override { log(f); }
	void warn(ILogger::LogFunction f) override { log(f); }
	void error(ILogger::LogFunction f) override { log(f); }
	const LogCache& getLogCache() const { return logCache; }
	void onTrackFilterChanged(int trackIndex, bool filterValue);
	bool isMuted(int trackIndex) const;
	TrackNames trackNames;
	const MutedTracks& getMutedTracks() const { return mutedTracks; }
	void initCompiler();
private:
	void startUdpSender(const juce::String &path);
	void stopUdpSender();
	double currentSheetTempoInSecondsPerQuarterNote = 0;
	bool compilerIsReady = false;
	MutedTracks mutedTracks;
	struct NoteOffStackItem
	{
		const juce::MidiMessage noteOff;
		int offsetInSamples = 0;
	};
	void findTrackName(size_t trackIndex, std::unordered_map<std::string, int> &trackAppearancesMap);
	typedef std::mutex Mutex;
	typedef std::list<NoteOffStackItem> NoteOffStack;
	typedef juce::MidiMessageSequence::MidiEventHolder const* const* MidiEventIterator;
	typedef std::vector<MidiEventIterator> IteratorTrackMap;
	PluginStateData pluginStateData;
	NoteOffStack noteOffStack;
	Mutex processMutex;
	FileWatcher fileWatcher;
	std::unique_ptr<funk::UdpSender> udpSender;
	void sendAllNoteOff(juce::MidiBuffer&);
	void updateFileWatcher(const CompiledSheet&);
	IteratorTrackMap _iteratorTrackMap;
	juce::MidiFile _midiFile;
	bool _lastIsPlayingState = false;
	void processNoteOffStack(juce::MidiBuffer& midiMessages);
	void applyMutedTrackState(int trackIndex);
	LogCache logCache;
	CompiledSheetPtr compiledSheet;
	double getTempoInSecondsPerQuarterNote(const juce::MidiFile &midiFile);
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginProcessor)
};
