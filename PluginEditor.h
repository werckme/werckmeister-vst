#pragma once

#include "PluginProcessor.h"
#include <memory>
#include "FilterComponent.h"
#include <mutex>

//==============================================================================
class PluginEditor  : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor (PluginProcessor&);
    virtual ~PluginEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;
    virtual void writeLine(const juce::String&);
    void tracksChanged();
private:
    bool tracksAreDirty = false;
    typedef std::recursive_mutex Mutex;
    Mutex mutex;
    std::list<juce::String> lineCache;
    std::unique_ptr<juce::FileChooser> myChooser;
    juce::TextEditor console;
    juce::TextButton findSheetFileBtn;
    juce::TextButton recompileBtn;
    juce::Viewport trackFilterView;
    FilterComponent trackFilter;
    PluginProcessor& processorRef;
    void selectSheetFile();
    void recompile();
    void onTrackFilterChanged(int trackIndex, bool filterValue);
    void setFilterStates();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginEditor)
};
