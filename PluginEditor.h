#pragma once

#include "PluginProcessor.h"

//==============================================================================
class AudioPluginAudioProcessorEditor  : public juce::AudioProcessorEditor
{
public:
    explicit AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor&);
    virtual ~AudioPluginAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    virtual void writeLine(const juce::String&);
private:
    std::unique_ptr<juce::FileChooser> myChooser;
    juce::TextEditor console;
    juce::TextButton findSheetFileBtn;
    juce::TextButton recompileBtn;
    AudioPluginAudioProcessor& processorRef;
    void selectSheetFile();
    void recompile();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioPluginAudioProcessorEditor)
};
