// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "profile_depository.hpp"
#include <map>
#include <cstring>
#include <boost/thread/shared_mutex.hpp>
#include "main_config.hpp"
#include "../atomic.hpp"
#include "../log.hpp"
#include "../profiler.hpp"

namespace Poseidon {

namespace {
	struct ProfileKey {
		const char *file;
		unsigned long line;
		const char *func;

		ProfileKey(const char *file_, unsigned long line_, const char *func_)
			: file(file_), line(line_), func(func_)
		{
		}

		bool operator<(const ProfileKey &rhs) const {
			const int fileCmp = std::strcmp(file, rhs.file);
			if(fileCmp != 0){
				return fileCmp < 0;
			}
			return line < rhs.line;
		}
	};

	struct ProfileCounters {
		volatile unsigned long long samples;
		volatile unsigned long long nsTotal;
		volatile unsigned long long nsExclusive;

		ProfileCounters()
			: samples(0), nsTotal(0), nsExclusive(0)
		{
		}
	};

	bool g_enabled = true;

	boost::shared_mutex g_mutex;
	std::map<ProfileKey, ProfileCounters> g_profile;
}

void ProfileDepository::start(){
	AUTO_REF(conf, MainConfig::getConfigFile());

	conf.get(g_enabled, "enable_profiler");
	LOG_POSEIDON_DEBUG("Enable profiler = ", g_enabled);
}
void ProfileDepository::stop(){
}

bool ProfileDepository::isEnabled(){
	return g_enabled;
}

void ProfileDepository::accumulate(const char *file, unsigned long line, const char *func,
	double total, double exclusive) NOEXCEPT
{
	try {
		std::map<ProfileKey, ProfileCounters>::iterator it;
		ProfileKey key(file, line, func);
		{
			const boost::shared_lock<boost::shared_mutex> slock(g_mutex);
			it = g_profile.find(key);
			if(it != g_profile.end()){
				goto _writeProfile;
			}
		}
		{
			const boost::unique_lock<boost::shared_mutex> ulock(g_mutex);
			it = g_profile.insert(std::make_pair(key, ProfileCounters())).first;
		}

	_writeProfile:
		atomicAdd(it->second.samples, 1, ATOMIC_RELAXED);
		atomicAdd(it->second.nsTotal, total * 1e9, ATOMIC_RELAXED);
		atomicAdd(it->second.nsExclusive, exclusive * 1e9, ATOMIC_RELAXED);

		LOG_POSEIDON_TRACE("Accumulated profile info: file = ", file, ", line = ", line,
			", func = ", func, ", total = ", total, " s, exclusive = ", exclusive, " s");
	} catch(...){
	}
}

std::vector<ProfileSnapshotItem> ProfileDepository::snapshot(){
	Profiler::flushProfilersInThread();

	std::vector<ProfileSnapshotItem> ret;
	{
		const boost::shared_lock<boost::shared_mutex> slock(g_mutex);
		ret.reserve(g_profile.size());
		for(AUTO(it, g_profile.begin()); it != g_profile.end(); ++it){
			ProfileSnapshotItem pi;
			pi.file = it->first.file;
			pi.line = it->first.line;
			pi.func = it->first.func;
			pi.samples = atomicLoad(it->second.samples, ATOMIC_RELAXED);
			pi.usTotal = atomicLoad(it->second.nsTotal, ATOMIC_RELAXED) / 1000;
			pi.usExclusive = atomicLoad(it->second.nsExclusive, ATOMIC_RELAXED) / 1000;
			ret.push_back(pi);
		}
	}
	return ret;
}

}
