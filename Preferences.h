#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <vector>

class Preferences : public juce::Component
{
public:
    Preferences();
    virtual ~Preferences() = default;
    virtual void paint (juce::Graphics&) override;
    virtual void resized() override;
private:
    juce::Label sheetPathLabelText_1;
    juce::Label sheetPathLabelText_2;
    juce::TextEditor sheetPath;
    juce::TextButton findPathBtn;
    juce::TextButton okBtn;
    std::unique_ptr<juce::FileChooser> myChooser;
    void select();
    void close();
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Preferences)
};
