// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "exception.hpp"
#include "../log.hpp"

namespace Poseidon {

CbppMessageException::CbppMessageException(const char *file, std::size_t line,
	CbppStatus status, SharedNts message)
	: ProtocolException(file, line, message, static_cast<unsigned>(status))
{
	LOG_POSEIDON_ERROR("CbppMessageException: status = ", status, ", what = ", what());
}
CbppMessageException::~CbppMessageException() NOEXCEPT {
}

}
