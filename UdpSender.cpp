#include "UdpSender.hpp"
#include <vector>

#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp> 
#include <boost/interprocess/shared_memory_object.hpp>
#include <boost/functional/hash.hpp>

namespace ip = boost::asio::ip;

namespace funk
{
	const int UdpSender::THREAD_IDLE_TIME = 50;
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
		juce::MemoryOutputStream ostream;
		auto jsonObj = new juce::DynamicObject();
		lastUpdateTimestamp = (unsigned long)time(NULL);
		jsonObj->setProperty("type", juce::var("werckmeister-vst-funk"));
		jsonObj->setProperty("sheetPath", juce::var(_sheetPath));
		jsonObj->setProperty("sheetTime", juce::var(currentTimeInQuarters));
		jsonObj->setProperty("instance", juce::var((juce::int64)this));
		jsonObj->setProperty("lastUpdateTimestamp", juce::var((juce::int64)lastUpdateTimestamp));
		jsonObj->setProperty("host", juce::var(hostDescription));
		CompiledSheetPtr sheet = compiledSheet.lock();
		if (!sheet) 
		{
			juce::JSON::writeToStream(ostream, jsonObj, true);
			return ostream.toString();
		}
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
		try 
		{
			runImpl();	
		}
		catch (const std::exception &ex)
		{
			_logger->error(LogLambda(log << "starting funkfeuer failed: " << ex.what()));
		}
		catch (...)
		{
			_logger->error(LogLambda(log << "starting funkfeuer failed: unkown error"));
		}
	}

	namespace 
	{
		class Lock
		{
		private:
			typedef boost::interprocess::named_mutex Mutex;
			std::unique_ptr<Mutex> _mutex;
			std::string _lockId;
			bool _isOnwer = false;
		public:
			Lock(const std::string& lockId) : _lockId(lockId)
			{
				using namespace boost::interprocess;
				_mutex = std::make_unique<Mutex>(open_or_create, _lockId.c_str());
			}
			~Lock()
			{
				if (!_isOnwer) {
					return;
				}
				_mutex->unlock();
				boost::interprocess::named_mutex::remove(_lockId.c_str());
			}
			bool tryLock()
			{
				_isOnwer = _mutex->try_lock();
				return _isOnwer;
			}
			inline bool isOnwer() const { return _isOnwer; }
		};
	}

	namespace 
	{
		juce::int64 getFileTimeStamp(const std::string& path)
		{
			try 
			{
				auto f = juce::File(path);
				return f.getLastModificationTime().toMilliseconds();
			} catch(...) 
			{
				return 0;
			}
		}
	}

	void UdpSender::runImpl() 
	{
		std::size_t mutexId = 0;
		auto timeStamp = getFileTimeStamp(_sheetPath);
		boost::hash_combine(mutexId, _sheetPath);
		boost::hash_combine(mutexId, _port);
		boost::hash_combine(mutexId, timeStamp);
		auto mutexIdString = std::to_string(mutexId);
		Lock lock(mutexIdString);
		bool isFree = lock.tryLock(); // only one instance should send per sheet file
		while (!threadShouldExit() && isFree)
		{
			auto fileChanged = getFileTimeStamp(_sheetPath) != timeStamp;
			if (fileChanged) 
			{
				break;
			}
			if (!_socket)
			{
				auto url = std::string("localhost:") + std::to_string(_port);
				try 
				{
					start(url);
				}
				catch(const std::exception &ex)
				{
					_logger->error(LogLambda(log << "starting funkfeuer failed: " << ex.what()));
					break;
				}
				catch(...)
				{
					_logger->error(LogLambda(log << "starting funkfeuer failed."));
					break;
				}
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
					break;
				}
				catch(...)
				{
					_logger->error(LogLambda(log << "funkfeuer failed."));
					break;
				}
			}
			sleep(THREAD_IDLE_TIME);
		}
		if (isFree)
		{
			stop();
		}
	}
}
