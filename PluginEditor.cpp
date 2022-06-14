#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <functional>
#include <iostream>
#include "Compiler.h"
extern "C" {
    #include "preferences_normal_png.h"
}

#define LOCK(mutex) std::lock_guard<Mutex> guard(mutex)

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    int w = 800, h = 600;
    setSize(w, h);

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
    auto prefrencesImage = juce::ImageCache::getFromMemory(preferences_normal_png_data, preferences_normal_png_size);
    
    preferences.setImages(true, true, true,
        prefrencesImage, 1.0f, juce::Colour(),
        prefrencesImage, 1.0f, juce::Colour(),
        prefrencesImage, 1.0f, juce::Colour());
    preferences.setBounds(w-50-5, 5, 50, 50);
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
    const auto &processorlogCache = processorRef.getLogCache();
    logCache.insert(logCache.end(), processorlogCache.begin(), processorlogCache.end());
    tracksChanged();
    triggerAsyncUpdate();
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

void PluginEditor::handleAsyncUpdate()
{
    LOCK(logMutex);
    std::stringstream ss;
    ss << console.getText().trim() << std::endl;
    for(const auto& line : logCache)
    {
        ss << line << std::endl;
    }
    console.setText(ss.str());
    logCache.clear();
    console.scrollDown();
}

void PluginEditor::writeLine(const juce::String& line)
{
    LOCK(logMutex);
    logCache.push_back(line.toStdString());
    triggerAsyncUpdate();
}

void PluginEditor::setFilterStates()
{
    for (size_t trackIndex = 0; trackIndex < trackFilter.getItems().size(); ++trackIndex)
    {
        auto state = !processorRef.isMuted((int)trackIndex);
        trackFilter.setItemState((int)trackIndex, state);
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