#include "PluginStateData.h"
#include <juce_data_structures/juce_data_structures.h>

namespace 
{
    const char * stateMagicCode = "fe56b86b-753a-413f-8917-f98c706901b5";
}

void writeStateData(const PluginStateData &stateData, juce::MemoryBlock &memoryBlock)
{
    juce::ValueTree valueTree("WerckmeisterVSTStateData");
    juce::MemoryOutputStream os(memoryBlock, true);
    juce::StringArray mutexTracksArray;
    for (const PluginStateData::TrackName& trackName : stateData.mutedTracks)
    {
        mutexTracksArray.add(trackName);
    }
    valueTree.setProperty("sheetPath", juce::var(stateData.sheetPath), nullptr);
    valueTree.setProperty("mutedTracks", juce::var(mutexTracksArray), nullptr);
    valueTree.setProperty("magicCode", juce::var(stateMagicCode), nullptr);
    valueTree.writeToStream(os);
}

PluginStateData readStateData(const void* data, int sizeInBytes)
{
    PluginStateData result;
    juce::ValueTree valueTree = juce::ValueTree::readFromData(data, sizeInBytes);
    if (!valueTree.isValid()) 
    {
        return result;
    }
    result.isValid = valueTree.getProperty("magicCode").toString() == juce::String(stateMagicCode);
    result.sheetPath = valueTree.getProperty("sheetPath").toString().toStdString();
    auto mutedTracksProperty = valueTree.getProperty("mutedTracks");
    if (mutedTracksProperty.isArray()) 
    {
        for (int i = 0; i < mutedTracksProperty.size(); ++i)
        {
            auto trackName = mutedTracksProperty[i].toString();
            result.mutedTracks.insert(trackName.toStdString());
        }
    }
    return result;
}