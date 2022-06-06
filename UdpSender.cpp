#include "UdpSender.hpp"
#include <vector>
#include <boost/bind.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/sync/named_mutex.hpp> 

namespace ip = boost::asio::ip;

namespace
{
	const int THREAD_IDLE_TIME = 50;
	const int THREAD_IDLE_TIME_WAITING = 500;
}

namespace funk
{
	UdpSender::UdpSender(const std::string &sheetPath) : juce::Thread("UDP sender"), _sheetPath(sheetPath) {}
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
	void UdpSender::run() 
	{
		using namespace boost::interprocess;
		auto sheetPath = juce::File::createLegalFileName(_sheetPath).toStdString(); // slashes in the mutex name seems to cause undefined behaviour
		named_mutex mutex(open_or_create, sheetPath.c_str());
		bool isFree = mutex.try_lock(); // only one instance should send per sheet file
		while (!threadShouldExit())
		{
			if (!isFree)
			{
				isFree = mutex.try_lock();
				if(!isFree) // maybe the former locking instance has been released
				{
					sleep(THREAD_IDLE_TIME_WAITING);
					continue;
				}
			}
			if (!_socket)
			{
				start("localhost:99192");
			}
			auto msg = "werckmeister-funk:" + messageToSend;
			send(msg.c_str(), msg.length());
			sleep(THREAD_IDLE_TIME);
		}
		if (isFree)
		{
			mutex.unlock();
			stop();
		}
	}
}
