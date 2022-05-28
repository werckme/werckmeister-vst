#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <functional>
#include <iostream>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);
    findSheetFileBtn.setButtonText("open sheet file");
    findSheetFileBtn.setBounds(100, 100, 150, 90);
    addAndMakeVisible(findSheetFileBtn);
    setSize (800, 600);
    findSheetFileBtn.onClick = std::bind(&AudioPluginAudioProcessorEditor::selectSheetFile, this);
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

void AudioPluginAudioProcessorEditor::paint (juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
}


void AudioPluginAudioProcessorEditor::selectSheetFile()
{
    using namespace juce;
    myChooser = std::make_unique<FileChooser> ("Find your sheet file",
                                               File::getSpecialLocation (File::userHomeDirectory),
                                               "*.sheet");
 
    auto folderChooserFlags = FileBrowserComponent::openMode;
 
    myChooser->launchAsync (folderChooserFlags, [this] (const FileChooser& chooser)
    {
        File sheetFile (chooser.getResult());
    }); 
}