#include "Compiler.h"
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <sstream>

#include <juce_core/juce_core.h>

#if WIN32
#define Popen _popen
#define PClose _pclose
#else
#define Popen popen
#define PClose pclose
#endif

namespace
{
    std::string exec(const std::string & cmd, const std::vector<std::string> &arguments) {
        std::stringstream ss;
        ss << cmd;
        for (auto const& arg : arguments)
        {
            ss << " " << arg;
        }

        std::array<char, 128> buffer;
        std::string result;
        std::unique_ptr<FILE, decltype(&PClose)> pipe(Popen(ss.str().c_str(), "r"), PClose);
        if (!pipe) 
        {
            throw std::runtime_error("popen() failed!");
        }
        while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) 
        {
            result += buffer.data();
        }
        return result.substr(0, result.length() - 1);
    }

    const juce::var & get(const juce::var &json, const std::string &key, bool expected = true) 
    {
        auto &result = json[key.c_str()];
        if (expected && result.isVoid())
        {
            throw std::exception("unexpected compiler response");
        }
        return result;
    }
}



CompiledSheet Compiler::compile(const std::string& sheetPath)
{
    CompiledSheet result;
    auto stringResult = exec(compilerExecutable(), { sheetPath, "--mode=json" });
    auto jsonResult = juce::JSON::parse(stringResult);
    const auto &midiInfo = get(jsonResult, "midi");
    // midi data
    const auto base64MidiData = get(midiInfo, "midiData").toString();
    double estimatedByteSize = (base64MidiData.length() * (3.0 / 4.0));
    juce::MemoryOutputStream midiByteStream((size_t)estimatedByteSize);
    juce::Base64::convertFromBase64(midiByteStream, base64MidiData);
    result.midiData.resize(midiByteStream.getDataSize());
    ::memcpy(result.midiData.data(), midiByteStream.getData(), result.midiData.size());
    // sources
    const auto& sources = get(midiInfo, "sources");
    for (int i = 0; i < sources.size(); ++i) 
    {
        const auto& sourceId = get(sources[i], "sourceId");
        const auto& path = get(sources[i], "path");
        result.sources.push_back({sourceId.toString().toStdString(), path.toString().toStdString()});
    }

    return result;
}

std::string Compiler::getVersionStr()
{
    return exec(compilerExecutable(), {"--version"});
}

std::string Compiler::compilerExecutable() const
{
    return "sheetc";
}