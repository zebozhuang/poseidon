// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SINGLETONS_EPOLL_DAEMON_HPP_
#define POSEIDON_SINGLETONS_EPOLL_DAEMON_HPP_

#include <boost/shared_ptr.hpp>
#include <boost/cstdint.hpp>
#include "../ip_port.hpp"

namespace Poseidon {

struct EpollSnapshotItem {
	IpPort remote;
	IpPort local;
	boost::uint64_t msOnline;
};

class TcpSessionBase;
class SocketServerBase;

struct EpollDaemon {
	static void start();
	static void stop();

	static boost::uint64_t getTcpRequestTimeout();

	static std::vector<EpollSnapshotItem> snapshot();

	static void addSession(const boost::shared_ptr<TcpSessionBase> &session);
	static void registerServer(boost::weak_ptr<const SocketServerBase> server);

private:
	EpollDaemon();
};

}

#endif
