#include "FilterComponent.h"

FilterComponent::FilterComponent()
{
	flexBox.flexDirection = juce::FlexBox::Direction::column;
	flexBox.flexWrap = juce::FlexBox::Wrap::wrap;
	flexBox.alignContent = juce::FlexBox::AlignContent::flexStart;
	flexBox.alignItems = juce::FlexBox::AlignItems::flexStart;
	flexBox.justifyContent = juce::FlexBox::JustifyContent::flexStart;
}

void FilterComponent::clear()
{
	for (const auto& control : filterControls)
	{
		removeChildComponent(control.get());
	}
	items.clear();
	filterControls.clear();
	flexBox.items.clear();
}

void FilterComponent::setItems(const Items& newItems)
{
	clear();
	this->items = newItems;
	for (int itemIndex = 0; itemIndex < items.size(); ++itemIndex)
	{
		const auto& item = items[itemIndex];
		FilterControlPtr btn = std::make_shared<FilterControl>(item);
		std::weak_ptr<FilterControl> wbtn = btn;
		btn->onClick = [this, itemIndex, wbtn]()
		{
			FilterControlPtr btn = wbtn.lock();
			if (btn == nullptr)
			{
				return;
			}
			onFilterChanged(itemIndex, btn->getToggleState());
		};
		btn->setToggleState(true, false);
		btn->setBounds(0, 0, 0, 20);
		btn->changeWidthToFitText();
		filterControls.push_back(btn);
		juce::FlexItem flexItem((float)btn->getWidth(), (float)btn->getHeight(), *btn);
		flexItem.margin = juce::FlexItem::Margin(5);
		flexBox.items.add(flexItem);
		addAndMakeVisible(*btn);
	}
	triggerAsyncUpdate();
}

void FilterComponent::handleAsyncUpdate()
{
	resized();
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