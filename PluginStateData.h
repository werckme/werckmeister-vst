#pragma once
#include <string>
#include <juce_core/juce_core.h>
#include <unordered_map>

struct PluginStateData 
{
    typedef int TrackIndex;
    typedef std::unordered_set<TrackIndex> MutedTracks;
    bool isValid = false;
    std::string sheetPath;
    MutedTracks mutedTracks;
};

void writeStateData(const PluginStateData&, juce::MemoryBlock&);
PluginStateData readStateData(const void* data, int sizeInBytes);