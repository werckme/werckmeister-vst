#pragma once

#include <string>
#include <vector>

struct Source
{
    std::string sourceId;
    std::string path;
};

struct CompiledSheet 
{
    std::vector<Source> sources;
    std::vector<unsigned char> midiData;
};

class Compiler 
{
public:
    CompiledSheet compile(const std::string &sheetPath);
    std::string getVersionStr();
private:
    std::string compilerExecutable() const;
};