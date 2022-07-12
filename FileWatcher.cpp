
#include "FileWatcher.hpp"


#define LOCK(mutex) std::lock_guard<Mutex> guard(mutex)


const int FileWatcher::THREAD_IDLE_TIME = 50;
FileWatcher::FileWatcher() : Thread("File Watcher Thread")
{
	setPriority(0);
}

void FileWatcher::run()
{
	while (!threadShouldExit())
	{
		checkFiles();
		sleep(THREAD_IDLE_TIME);
	}
}

void FileWatcher::setFileList(const FileList& fileList)
{
	LOCK(mutex);
	lastUpdatedMap.clear();
	for (const auto& file : fileList)
	{
		auto timeStamp = getTimeStamp(file);
		lastUpdatedMap.insert({file, timeStamp});
	}
}

void FileWatcher::handleAsyncUpdate()
{
	onFileChanged();
}

void FileWatcher::checkFiles()
{
	LOCK(mutex);
	lastChangedFile = "";
	for (auto& fileTimeStampPair : lastUpdatedMap)
	{
		auto timeStamp = getTimeStamp(fileTimeStampPair.first);
		if (timeStamp == fileTimeStampPair.second)
		{
			continue;
		}
		fileTimeStampPair.second = timeStamp;
		lastChangedFile = fileTimeStampPair.first;
	}
	if (!lastChangedFile.empty())
	{
		triggerAsyncUpdate();
	}
}

FileWatcher::TimeStamp FileWatcher::getTimeStamp(const std::string& filePath) const
{
	juce::File file(filePath);
	if (!file.exists())
	{
		throw std::runtime_error("file not found:" + filePath);
	}
	return file.getLastModificationTime().toMilliseconds();
}