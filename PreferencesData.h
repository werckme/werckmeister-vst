#pragma once
#include <string>
#include <juce_core/juce_core.h>
#include <unordered_map>

struct PreferencesData 
{
    std::string binPath;
    int funkfeuerPort = 99192;
};

void writePreferencesData(const PreferencesData&);
PreferencesData readPreferencesData();