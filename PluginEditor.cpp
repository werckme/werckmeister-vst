#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <functional>
#include <iostream>

//==============================================================================
AudioPluginAudioProcessorEditor::AudioPluginAudioProcessorEditor (AudioPluginAudioProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize(800, 600);

    //
    findSheetFileBtn.setButtonText("Open Sheet File");
    findSheetFileBtn.setBounds(5, 5, 150, 50);
    addAndMakeVisible(findSheetFileBtn);
    findSheetFileBtn.onClick = std::bind(&AudioPluginAudioProcessorEditor::selectSheetFile, this);
    
    //
    recompileBtn.setButtonText("Recompile!");
    recompileBtn.setBounds(165, 5, 150, 50);
    addAndMakeVisible(recompileBtn);
    recompileBtn.onClick = std::bind(&AudioPluginAudioProcessorEditor::recompile, this);

    //
    console.setBounds(5, 60, getWidth() - 5 - 5, getHeight() - 60 - 5);
    console.setMultiLine(true);
    console.setEnabled(false);
    addAndMakeVisible(console);
    
    //
    writeLine(juce::String("Werckmeister VST ") + JucePlugin_VersionString);
    const auto &logCache = processorRef.getLogCache();
    for (const auto& str : logCache)
    {
        writeLine(str);
    }
}

AudioPluginAudioProcessorEditor::~AudioPluginAudioProcessorEditor()
{
}

void AudioPluginAudioProcessorEditor::paint(juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void AudioPluginAudioProcessorEditor::resized()
{
}

void AudioPluginAudioProcessorEditor::recompile()
{
    processorRef.reCompile();
}

void AudioPluginAudioProcessorEditor::selectSheetFile()
{
    using namespace juce;
    myChooser = std::make_unique<FileChooser> ("Find your sheet file",
                                               File(),
                                               "*.sheet");
 
    auto folderChooserFlags = FileBrowserComponent::openMode;
 
    myChooser->launchAsync (folderChooserFlags, [this] (const FileChooser& chooser)
    {
        File sheetFile (chooser.getResult());
        processorRef.compile(sheetFile.getFullPathName());
    }); 
}

void AudioPluginAudioProcessorEditor::writeLine(const juce::String& line)
{
    auto newLine = (console.getText() + "\n" + line).trim();
    console.setText(newLine);
    console.scrollDown();
}