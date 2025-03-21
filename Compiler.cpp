#include "Compiler.h"
#include <juce_core/juce_core.h>
#include <cstdio>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <vector>
#include <sstream>
#include "PreferencesData.h"


#if WIN32
#define Popen _popen
#define PClose _pclose
#else
#define Popen popen
#define PClose pclose
#endif

namespace
{
    std::string __compiler_executable;
}

namespace
{
    class CompilerException : public std::exception {
    public:
        CompilerException(const std::string &what) : _what(what) {}
        virtual const char* what() const throw()
        {
            return _what.c_str();
        }
    private:
        const std::string _what;
    };
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
            throw std::runtime_error("unexpected compiler response");
        }
        return result;
    }

    void checkForErrors(const juce::var& jsonResult, const std::string &, const std::string&)
    {
        const auto& errorMessage = get(jsonResult, "errorMessage", false);
        if (errorMessage.isVoid())
        {
            return;
        }
        const auto& sourceFile = get(jsonResult, "sourceFile", false);
        const auto& position = get(jsonResult, "positionBegin", false);
        std::stringstream ss;
        if (!sourceFile.isVoid())
        {
            ss << "in file \""<< sourceFile.toString() << "\"";
        }
        if (!position.isVoid())
        {
            ss << ": position " << position.toString();
        }
        ss << "\n" << errorMessage.toString();
        throw CompilerException(ss.str());
    }
}


CompiledSheetPtr Compiler::compile(const std::string& sheetPath)
{
    auto compilerExe = compilerExecutable();
    logger.log(LogLambda(log << "sheetc" << " \"" << sheetPath << "\""));
    try 
    {
        CompiledSheetPtr result = std::make_shared<CompiledSheet>();
        auto stringResult = exec(compilerExe, { sheetPath, "--mode=json" });
        auto jsonResult = juce::JSON::parse(stringResult);
        checkForErrors(jsonResult, compilerExe, sheetPath);
        const auto& midiInfo = get(jsonResult, "midi");
        // midi data
        const auto base64MidiData = get(midiInfo, "midiData").toString();
        double estimatedByteSize = (base64MidiData.length() * (3.0 / 4.0) + 10);
        juce::MemoryOutputStream midiByteStream((size_t)estimatedByteSize);
        juce::Base64::convertFromBase64(midiByteStream, base64MidiData);
        result->midiData.resize(midiByteStream.getDataSize());
        ::memcpy(result->midiData.data(), midiByteStream.getData(), result->midiData.size());
        logger.log(LogLambda(log << "MIDI data created: " << result->midiData.size() << " Bytes"));
        // sources
        const auto& sources = get(midiInfo, "sources");
        for (int i = 0; i < sources.size(); ++i)
        {
            const auto& sourceId = get(sources[i], "sourceId");
            const auto& path = get(sources[i], "path");
            result->sources.push_back({ sourceId.toString().toStdString(), path.toString().toStdString() });
        }
        // document event infos
        const auto& eventInfos = get(jsonResult, "eventInfos");
        for (int i = 0; i < eventInfos.size(); ++i)
        {
            const auto& sheetEventInfos = get(eventInfos[i], "sheetEventInfos");
            for(int j = 0; j < sheetEventInfos.size(); ++j)
            {
                DocumentEventInfo docEventInfo;
                const auto &sheetEventInfo = sheetEventInfos[j];
                docEventInfo.sourceId = (juce::int64)get(sheetEventInfo, "sourceId");
                docEventInfo.beginPosition = (juce::int64)get(sheetEventInfo, "beginPosition");
                auto endp = get(sheetEventInfo, "endPosition", false);
                if (!endp.isVoid()) 
                {
                    docEventInfo.endPosition = (juce::int64)get(sheetEventInfo, "endPosition", false);
                } else {
                    docEventInfo.beginPosition;
                }
                docEventInfo.beginTime = (double)get(sheetEventInfo, "beginTime");
                docEventInfo.endTime = (double)get(sheetEventInfo, "endTime");
                EventPositionSet value = {docEventInfo};
                result->eventInfos += std::make_pair(TimelineIntervalType::right_open(docEventInfo.beginTime, docEventInfo.endTime), value);
            }
        }
        return result;
    }
    catch (const CompilerException& ex)
    {
        logger.error(LogLambda(log << ex.what()));
    }
    catch (const std::exception& ex)
    {
        logger.error(LogLambda(log << "FAILED: " << ex.what()));
    }
    catch (...)
    {
        logger.error(LogLambda(log << "FAILED: unkown error"));
    }
    return nullptr;

}

std::string Compiler::getVersionStr()
{
    return exec(compilerExecutable(), {"--version"});
}

std::string Compiler::compilerExecutable() const
{
    if (__compiler_executable.empty())
    {
        auto preferencesData = readPreferencesData();
        if (!preferencesData.binPath.empty())
        {
            __compiler_executable = (juce::File::addTrailingSeparator(preferencesData.binPath) + "sheetc").toStdString();
            return __compiler_executable;
        }
#ifdef WIN32
        __compiler_executable = "sheetc";
#else
        __compiler_executable = exec("which", {"sheetc"});
        if (__compiler_executable.empty())
        {
            __compiler_executable = "/usr/local/bin/sheetc";
        }
#endif
    }
    return __compiler_executable;
}

void Compiler::resetExecutablePath()
{
    __compiler_executable.clear();
}