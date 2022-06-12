#include "PreferencesData.h"
#include <juce_data_structures/juce_data_structures.h>

namespace
{
    const char * WMConfigBasePath = "Werckmeister-VST";
    const char * WMConfigFile = "preferences.xml";
    juce::String getConfigFile()
    {
        auto basePath = juce::File::getSpecialLocation(juce::File::userApplicationDataDirectory).getFullPathName();
        basePath = juce::File::addTrailingSeparator(basePath);
        basePath += WMConfigBasePath;
        basePath = juce::File::addTrailingSeparator(basePath);
        basePath += WMConfigFile;
        return basePath;
    }
}

void writePreferencesData(const PreferencesData& data)
{
    auto configFile = juce::File(getConfigFile());
    if (!configFile.exists())
    {
        configFile.create();
    }
    juce::ValueTree valueTree("WerckmeisterVSTPreferencesData");
    valueTree.setProperty("binPath", juce::var(data.binPath), nullptr);
    valueTree.setProperty("funkfeuerPort", juce::var(data.funkfeuerPort), nullptr);
    configFile.replaceWithText(valueTree.toXmlString());
}

PreferencesData readPreferencesData()
{
    PreferencesData result;
    auto configFile = juce::File(getConfigFile());
    if (!configFile.exists())
    {
        return result;
    }
    auto valueTree = juce::ValueTree::fromXml(configFile.loadFileAsString());
    result.binPath = valueTree.getProperty("binPath").toString().toStdString();
    auto portProperty = valueTree.getProperty("funkfeuerPort");
    if (!portProperty.isVoid()) {
         result.funkfeuerPort = (int)valueTree.getProperty("funkfeuerPort");
    }
    return result;
}