// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_SOCKET_SERVER_BASE_HPP_
#define POSEIDON_SOCKET_SERVER_BASE_HPP_

#include "cxx_util.hpp"
#include "raii.hpp"
#include "ip_port.hpp"

namespace Poseidon {

class SocketServerBase : NONCOPYABLE {
private:
	const UniqueFile m_socket;
	const IpPort m_localInfo;

public:
	explicit SocketServerBase(UniqueFile socket);
	virtual ~SocketServerBase();

protected:
	int getFd() const {
		return m_socket.get();
	}

public:
	const IpPort &getLocalInfo() const {
		return m_localInfo;
	}

	virtual bool poll() const = 0;
};

}

#endif
