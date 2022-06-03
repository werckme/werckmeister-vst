#pragma once

#include <string>
#include <vector>
#include "ILogger.h"

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
    Compiler(ILogger &logger_) : logger(logger_) {}
    CompiledSheet compile(const std::string &sheetPath);
    std::string getVersionStr();
private:
    ILogger& logger;
    inline std::string compilerExecutable() const;
};