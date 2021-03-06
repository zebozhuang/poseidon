// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#ifndef POSEIDON_HTTP_UPGRADED_SESSION_BASE_HPP_
#define POSEIDON_HTTP_UPGRADED_SESSION_BASE_HPP_

#include "../session_base.hpp"
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/weak_ptr.hpp>
#include <boost/function.hpp>

namespace Poseidon {

class OptionalMap;
class HttpSession;

class HttpUpgradedSessionBase : public SessionBase {
	friend class HttpSession;

private:
	const boost::weak_ptr<HttpSession> m_parent;

protected:
	explicit HttpUpgradedSessionBase(const boost::shared_ptr<HttpSession> &parent);

private:
	virtual void onInitContents(const void *data, std::size_t size) = 0;
	virtual void onReadAvail(const void *data, std::size_t size) = 0;
	virtual void onClose() NOEXCEPT OVERRIDE FINAL;

public:
	bool send(StreamBuffer buffer, bool fin = false) OVERRIDE FINAL;
	bool hasBeenShutdown() const OVERRIDE FINAL;
	bool forceShutdown() NOEXCEPT OVERRIDE FINAL;

	boost::shared_ptr<const HttpSession> getParent() const {
		return m_parent.lock();
	}
	boost::shared_ptr<HttpSession> getParent(){
		return m_parent.lock();
	}

	// 以下所有函数，如果原来的 HttpSession 被删除，抛出 bad_weak_ptr。
	boost::shared_ptr<const HttpSession> getSafeParent() const {
		return boost::shared_ptr<const HttpSession>(m_parent);
	}
	boost::shared_ptr<HttpSession> getSafeParent(){
		return boost::shared_ptr<HttpSession>(m_parent);
	}

	std::size_t getCategory() const;
	const std::string &getUri() const;
	const OptionalMap &getGetParams() const;
	const OptionalMap &getHeaders() const;

	void setTimeout(unsigned long long timeout);
	void registerOnClose(boost::function<void ()> callback);
};

}

#endif
