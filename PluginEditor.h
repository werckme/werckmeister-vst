#pragma once

#include "PluginProcessor.h"
#include <memory>
#include "FilterComponent.h"
#include <mutex>
#include "Preferences.h"
#include <list>
#include <string>

//==============================================================================
class PluginEditor  : public juce::AudioProcessorEditor, public juce::AsyncUpdater
{
public:
    explicit PluginEditor (PluginProcessor&);
    virtual ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    virtual void writeLine(const juce::String&);
    void tracksChanged();
    void handleAsyncUpdate() override;
private:
    std::list<std::string> logCache;
    bool tracksAreDirty = false;
    typedef std::recursive_mutex Mutex;
    Mutex logMutex;
    std::unique_ptr<juce::FileChooser> myChooser;
    juce::TextEditor console;
    juce::TextButton findSheetFileBtn;
    juce::TextButton recompileBtn;
    juce::ImageButton preferences;
    juce::ImageComponent background;
    juce::Viewport trackFilterView;
    std::unique_ptr<Preferences> preferencesComponent;
    FilterComponent trackFilter;
    PluginProcessor& processorRef;
    void selectSheetFile();
    void recompile();
    void showPreferences();
    void onTrackFilterChanged(int trackIndex, bool filterValue);
    void setFilterStates();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
