#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <functional>
#include <iostream>
#include "Compiler.h"


//==============================================================================
PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    setSize(800, 600);

    //
    findSheetFileBtn.setButtonText("Open Sheet File");
    findSheetFileBtn.setBounds(5, 5, 150, 50);
    addAndMakeVisible(findSheetFileBtn);
    findSheetFileBtn.onClick = std::bind(&PluginEditor::selectSheetFile, this);
    
    //
    recompileBtn.setButtonText("Recompile!");
    recompileBtn.setBounds(165, 5, 150, 50);
    addAndMakeVisible(recompileBtn);
    recompileBtn.onClick = std::bind(&PluginEditor::recompile, this);

    //
    preferences.setButtonText("S");
    preferences.setBounds(720, 5, 50, 50);
    preferences.onClick = std::bind(&PluginEditor::showPreferences, this);
    addAndMakeVisible(preferences);

    //
    trackFilterView.setViewedComponent(&trackFilter, false);
    trackFilter.setBounds(5, 60, getWidth() - 5 - 5, 92);
    trackFilter.onFilterChanged = std::bind(&PluginEditor::onTrackFilterChanged, this, std::placeholders::_1, std::placeholders::_2);
    trackFilterView.setBounds(5, 60, getWidth() - 5 - 5, 100);
    addAndMakeVisible(trackFilterView);

    //
    console.setBounds(5, 60 + 100 + 5, getWidth() - 5 - 5, getHeight() - 200);
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

    tracksChanged();
}

void PluginEditor::onTrackFilterChanged(int trackIndex, bool filterValue)
{
    processorRef.onTrackFilterChanged(trackIndex, filterValue);
}

void PluginEditor::tracksChanged()
{
    trackFilter.setItems(processorRef.trackNames);
    this->setBounds(getBounds());
    setFilterStates();
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint(juce::Graphics& g)
{
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
}

void PluginEditor::resized()
{
    trackFilterView.setBounds(trackFilterView.getX(), trackFilterView.getY(), trackFilter.getWidth(), trackFilterView.getHeight());
}

void PluginEditor::recompile()
{
    processorRef.reCompile();
}

void PluginEditor::selectSheetFile()
{
    using namespace juce;
    myChooser = std::make_unique<FileChooser> ("Find your sheet file",
                                               File(),
                                               "*.sheet");
 
    auto folderChooserFlags = FileBrowserComponent::openMode | FileBrowserComponent::canSelectFiles;
 
    myChooser->launchAsync (folderChooserFlags, [this] (const FileChooser& chooser)
    {
        File sheetFile (chooser.getResult());
        processorRef.compile(sheetFile.getFullPathName());
    }); 
}

void PluginEditor::writeLine(const juce::String& line)
{
    auto newLine = (console.getText() + "\n" + line).trim();
    console.setText(newLine);
    console.scrollDown();
}

void PluginEditor::setFilterStates()
{
    for (int trackIndex = 0; trackIndex < trackFilter.getItems().size(); ++trackIndex)
    {
        auto state = !processorRef.isMuted(trackIndex);
        trackFilter.setItemState(trackIndex, state);
    }
}

void PluginEditor::showPreferences()
{
    if (!preferencesComponent)
    {
        preferencesComponent = std::make_unique<Preferences>();
        preferencesComponent->onPreferencesChanged = [this]()
        {
            processorRef.initCompiler();
            processorRef.reCompile();
        };
    }
    preferencesComponent->loadPreferences();
    juce::DialogWindow::LaunchOptions lauchOptions;
    lauchOptions.dialogTitle = "Werckmeister VST Preferences";
    lauchOptions.content = juce::OptionalScopedPointer<Component>(preferencesComponent.get(), false);
    lauchOptions.dialogBackgroundColour = findColour(juce::ResizableWindow::backgroundColourId, true);
    lauchOptions.resizable = false;
    lauchOptions.useNativeTitleBar = false;
    lauchOptions.launchAsync();
}