#include "FilterComponent.h"

FilterComponent::FilterComponent()
{
	flexBox.flexDirection = juce::FlexBox::Direction::column;
	flexBox.flexWrap = juce::FlexBox::Wrap::wrap;
	flexBox.alignContent = juce::FlexBox::AlignContent::flexStart;
	flexBox.alignItems = juce::FlexBox::AlignItems::flexStart;
	flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
	for (int i = 0; i < 50; ++i) items.push_back("Track " + std::to_string(i));
	for (const auto& item : items)
	{
		FilterControlPtr btn = std::make_shared<FilterControl>(item);
		btn->setBounds(0, 0, 0, 20);
		btn->changeWidthToFitText();
		filterControls.push_back(btn);
		juce::FlexItem flexItem(btn->getWidth(), btn->getHeight(), *btn);
		flexItem.margin = juce::FlexItem::Margin(5);
		flexBox.items.add(flexItem);
		addAndMakeVisible(*btn);
	}
}

void FilterComponent::paint(juce::Graphics&)
{
}

void FilterComponent::resized() 
{
	auto bounds = juce::Rectangle<int>(0, 0, getWidth(), getHeight());
	flexBox.performLayout(bounds);
	if (filterControls.empty()) 
	{
		return;
	}
	bounds = getBounds();
	auto lastControl = filterControls.back();
	// works only in column mode
	bounds.setWidth(lastControl->getX() + lastControl->getWidth());
	setBounds(bounds);
}