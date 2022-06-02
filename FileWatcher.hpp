#pragma once

#include <juce_core/juce_core.h>
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <juce_gui_basics/juce_gui_basics.h>

class FileWatcher : public juce::Thread, juce::AsyncUpdater
{
public:
	FileWatcher();
	typedef std::vector<std::string> FileList;
	typedef std::function<void()> FileChangedHandler;
	FileChangedHandler onFileChanged = [](){};
	void setFileList(const FileList& fileList);
	void run() override;
	void handleAsyncUpdate() override;
private:
	typedef std::mutex Mutex;
	typedef juce::int64 TimeStamp;
	typedef std::unordered_map<std::string, TimeStamp> LastUpdatedMap;
	std::string lastChangedFile;
	LastUpdatedMap lastUpdatedMap;
	Mutex mutex;
	void checkFiles();
	TimeStamp getTimeStamp(const std::string& filePath) const;
	
};
