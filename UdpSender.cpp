#include "UdpSender.hpp"
#include <vector>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp> 
#include <boost/interprocess/shared_memory_object.hpp>

namespace ip = boost::asio::ip;

namespace
{
	const int THREAD_IDLE_TIME = 50;
	const int THREAD_IDLE_TIME_WAITING = 500;
}

namespace funk
{
	UdpSender::UdpSender(ILogger *logger, const std::string &sheetPath, int port) : 
		juce::Thread("UDP sender"), 
		_sheetPath(sheetPath), 
		_logger(logger), 
		_port(port) {}
	void UdpSender::start(const std::string &hostStr)
	{
		ip::udp::resolver resolver(_service);
		auto hostInfo = getHostInfo(hostStr);
		ip::udp::resolver::query query(ip::udp::v4(), std::get<0>(hostInfo), std::get<1>(hostInfo));
		_endpoint = *resolver.resolve(query);
		_socket = std::move(SocketPtr(new Socket(_service)));
		_socket->open(ip::udp::v4());
	}
	void UdpSender::stop()
	{
		_socket->close();
		_socket.reset();
	}
	void UdpSender::send(const char *bytes, size_t length)
	{
		if (!_socket || !_socket->is_open())
		{
			throw std::runtime_error("tried to send via a closed socked");
		}
		namespace pc = boost::asio::placeholders;
		_socket->send_to(boost::asio::buffer(bytes, length), _endpoint);
	}
	std::tuple<UdpSender::Host, UdpSender::Port>
	UdpSender::getHostInfo(const std::string &str) const
	{
		std::vector<std::string> strs;
		boost::split(strs, str, boost::is_any_of(":"));
		if (strs.size() < 2)
		{
			return std::make_tuple(str, "");
		}
		return std::make_tuple(strs[0], strs[1]);
	}
	
	juce::String UdpSender::createMessage()
	{
		CompiledSheetPtr sheet = compiledSheet.lock();
		if (!sheet) 
		{
			return "";
		}
		juce::MemoryOutputStream ostream;
		auto jsonObj = new juce::DynamicObject();
		lastUpdateTimestamp = (unsigned long)time(NULL);
		jsonObj->setProperty("type", juce::var("werckmeister-vst-funk"));
		jsonObj->setProperty("sheetPath", juce::var(_sheetPath));
		jsonObj->setProperty("sheetTime", juce::var(currentTimeInQuarters));
		jsonObj->setProperty("lastUpdateTimestamp", juce::var((double)lastUpdateTimestamp));
		const auto &timeline = sheet->eventInfos;
		auto it = timeline.find(currentTimeInQuarters);
		if (it == timeline.end())
		{
			juce::JSON::writeToStream(ostream, jsonObj, true);
			return ostream.toString();
		}
		juce::Array<juce::var> eventInfos;
		for (const auto &ev : it->second)
		{
			auto eventInfo = new juce::DynamicObject();
			eventInfo->setProperty("sourceId", juce::var((juce::int64)ev.sourceId));
			eventInfo->setProperty("beginPosition", juce::var(ev.beginPosition));
			eventInfo->setProperty("endPosition", juce::var(ev.endPosition));
			eventInfo->setProperty("beginTime", juce::var(ev.beginTime));
			eventInfo->setProperty("endTime", juce::var(ev.endTime));
			eventInfos.add(eventInfo);
		}
		jsonObj->setProperty("sheetEventInfos", juce::var(eventInfos));
		juce::JSON::writeToStream(ostream, jsonObj, true);
		lastSentEvent = it;
		return ostream.toString();
	}  

	void UdpSender::run() 
	{
		using namespace boost::interprocess;
		auto sheetPath = juce::File::createLegalFileName(_sheetPath).toStdString(); // slashes in the mutex name seems to cause undefined behaviour
		//named_mutex mutex(open_or_create, sheetPath.c_str());
		bool isFree = true; //mutex.try_lock(); // only one instance should send per sheet file
		auto url = std::string("localhost:") + std::to_string(_port);
		_logger->info(LogLambda(log << "starting funkfeuer on " << url));
		while (!threadShouldExit())
		{
			if (!isFree)
			{
				// isFree = mutex.try_lock();
				if(!isFree) // maybe the former locking instance has been released
				{
					sleep(THREAD_IDLE_TIME_WAITING);
					continue;
				}
			}
			if (!_socket)
			{
				start(url);
				_logger->info(LogLambda(log << "funkfeuer on " << url));
			}
			auto msg = createMessage();
			if (!msg.isEmpty())
			{
				try 
				{
					send(msg.toRawUTF8(), msg.getNumBytesAsUTF8());
				} 
				catch(const std::exception &ex) 
				{
					_logger->error(LogLambda(log << "funkfeuer failed:" << ex.what()));
				}
			}
			sleep(THREAD_IDLE_TIME);
		}
		if (isFree)
		{
			//mutex.unlock();
			stop();
		}
	}
}
