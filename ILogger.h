#pragma once

#include <ostream>
#include <functional>

#define LogLambda(x) [&](auto &log) { (x); }

class ILogger 
{
public:
    typedef std::ostream Stream;
    typedef std::function<void(Stream&)> LogFunction;
    virtual void log(LogFunction) = 0;
    virtual void info(LogFunction) = 0;
    virtual void warn(LogFunction) = 0;
    virtual void error(LogFunction) = 0;
};