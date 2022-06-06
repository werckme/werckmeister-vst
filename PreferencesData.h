#pragma once
#include <string>
#include <juce_core/juce_core.h>
#include <unordered_map>

struct PreferencesData 
{
    std::string binPath;
};

void writePreferencesData(const PreferencesData&);
PreferencesData readPreferencesData();