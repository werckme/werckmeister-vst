#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>
#include "PreferencesData.h"
#include <functional>

class Preferences : public juce::Component, public juce::AsyncUpdater
{
public:
    typedef juce::Component Base;
    typedef std::function<void()> PreferencesChangedHandler;
    Preferences();
    virtual ~Preferences() = default;
    virtual void paint (juce::Graphics&) override;
    virtual void resized() override;
    PreferencesData preferencesData;
    PreferencesChangedHandler onPreferencesChanged = [](){};
    void loadPreferences();
    void handleAsyncUpdate() override;
private:
    juce::Label sheetPathLabelText_1;
    juce::Label sheetPathLabelText_2;
    juce::TextEditor sheetPath;
    juce::TextButton findPathBtn;
    juce::TextButton okBtn;
    std::unique_ptr<juce::FileChooser> myChooser;
    void select();
    void close();
    void apply();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Preferences)
};
