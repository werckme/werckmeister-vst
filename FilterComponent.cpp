#include "FilterComponent.h"
#include <algorithm>
//#include "TrackColors.h"

namespace 
{
	const std::array<juce::Colour, 17> TrackColors = {
		juce::Colour((juce::uint8)255,(juce::uint8)255,(juce::uint8)255),
		juce::Colour((juce::uint8)111,(juce::uint8)105,(juce::uint8)172),
		juce::Colour((juce::uint8)149,(juce::uint8)218,(juce::uint8)193),
		juce::Colour((juce::uint8)255,(juce::uint8)235,(juce::uint8)161),
		juce::Colour((juce::uint8)253,(juce::uint8)111,(juce::uint8)150),
		juce::Colour((juce::uint8)61,(juce::uint8)178,(juce::uint8)255),
		juce::Colour((juce::uint8)255,(juce::uint8)184,(juce::uint8)48),
		juce::Colour((juce::uint8)255,(juce::uint8)36,(juce::uint8)66),
		juce::Colour((juce::uint8)255,(juce::uint8)72,(juce::uint8)72),
		juce::Colour((juce::uint8)172,(juce::uint8)102,(juce::uint8)204),
		juce::Colour((juce::uint8)243,(juce::uint8)113,(juce::uint8)33),
		juce::Colour((juce::uint8)245,(juce::uint8)180,(juce::uint8)97),
		juce::Colour((juce::uint8)217,(juce::uint8)32,(juce::uint8)39),
		juce::Colour((juce::uint8)188,(juce::uint8)101,(juce::uint8)141),
		juce::Colour((juce::uint8)141,(juce::uint8)68,(juce::uint8)139),
		juce::Colour((juce::uint8)223,(juce::uint8)14,(juce::uint8)98),
		juce::Colour((juce::uint8)255,(juce::uint8)46,(juce::uint8)76)
	};


	std::string toLower(const std::string& str)
	{
		auto copy = str;
		std::transform(copy.begin(), copy.end(), copy.begin(), ::tolower);
		return copy;
	}

	bool contains(const std::string& toSearch, const std::string& toFind)
	{
		return toLower(toSearch).find(toFind) < toSearch.length();
	}
}

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
	int colorCounter = 0;
	for (int itemIndex = 0; itemIndex < items.size(); ++itemIndex)
	{
		const auto& item = items[itemIndex];
		FilterControlPtr btn = std::make_shared<FilterControl>(item);
		auto color = TrackColors.at(colorCounter++ % TrackColors.size());
		btn->setColour(FilterControl::ColourIds::tickColourId, color);
		btn->setColour(FilterControl::ColourIds::textColourId, color);
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
		bool doNotShow = ::contains(item, "master track");
		doNotShow |= ::contains(item, "unnamed track");
		if (doNotShow)
		{
			continue;
		}
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

void FilterComponent::setItemState(int itemIndex, bool state)
{
	auto ctrl = filterControls.at(itemIndex);
	ctrl->setToggleState(state, false);
}