#pragma once

#include <tuple>
#include <memory>
#include <vector>
#include <boost/asio.hpp>
#include <boost/core/noncopyable.hpp>
#include <juce_core/juce_core.h>

namespace funk
{
	/**
	 * test the connection using: socat UDP-RECV:$port STDOUT
	 */
	class UdpSender : boost::noncopyable, public juce::Thread
	{
	public:
		void start(const std::string &host);
		void stop();
		void send(const char *bytes, size_t length);

	private:
		typedef std::string Host;
		typedef std::string Port;
		std::tuple<Host, Port> getHostInfo(const std::string &str) const;
		typedef boost::asio::ip::udp::socket Socket;
		typedef std::unique_ptr<Socket> SocketPtr;
		typedef boost::asio::ip::udp::endpoint Endpoint;
		typedef std::unique_ptr<Endpoint> EndpointPtr;
		typedef boost::system::error_code ErrorCode;
		typedef boost::asio::io_service Service;
		Service _service;
		SocketPtr _socket;
		Endpoint _endpoint;
	protected:
	public:
		std::string messageToSend;
		UdpSender();
		virtual ~UdpSender() = default;
		virtual void run() override;
	};
}
