
#pragma once

#include <juce_core/juce_core.h>
#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <vector>

//==============================================================================
class FilterComponent : public juce::Component, public juce::AsyncUpdater
{
public:
    typedef std::vector<std::string> Items;
    typedef juce::ToggleButton FilterControl;
    typedef std::shared_ptr<FilterControl> FilterControlPtr;
    FilterComponent();
    virtual ~FilterComponent() = default;
    void paint (juce::Graphics&) override;
    void resized() override;
    void setItems(const Items& items);
    void clear();
    const Items& getItems() const { return items; }
private:
    void handleAsyncUpdate() override;
    juce::FlexBox flexBox;
    typedef std::vector<FilterControlPtr> FilterControls;
    Items items;
    FilterControls filterControls;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (FilterComponent)
};
