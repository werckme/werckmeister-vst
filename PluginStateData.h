#pragma once
#include <string>
#include <juce_core/juce_core.h>
#include <vector>

struct PluginStateData 
{
    bool isValid = false;
    std::string sheetPath;
    std::vector<int> mutedTracks;
};

void writeStateData(const PluginStateData&, juce::MemoryBlock&);
PluginStateData readStateData(const void* data, int sizeInBytes);