#pragma once
#include <string>
#include <juce_core/juce_core.h>

struct PluginStateData 
{
    bool isValid = false;
    std::string sheetPath;
};

void writeStateData(const PluginStateData&, juce::MemoryBlock&);
PluginStateData readStateData(const void* data, int sizeInBytes);