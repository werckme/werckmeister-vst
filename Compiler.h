#pragma once

#include <string>
#include <vector>
#include "ILogger.h"
#include "CompiledSheet.h"

class Compiler 
{
public:
    Compiler(ILogger &logger_) : logger(logger_) {}
    CompiledSheetPtr compile(const std::string &sheetPath);
    std::string getVersionStr();
    std::string compilerExecutable() const;
    void resetExecutablePath();
private:
    ILogger& logger;
    
};