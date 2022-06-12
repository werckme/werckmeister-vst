#pragma once
#include <string>
#include <juce_core/juce_core.h>
#include <unordered_map>


namespace 
{
    static const int DefaultPort = 7935;
}

struct PreferencesData 
{
    std::string binPath;
    int funkfeuerPort = DefaultPort;
};

void writePreferencesData(const PreferencesData&);
PreferencesData readPreferencesData();