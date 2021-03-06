// 这个文件是 Poseidon 服务器应用程序框架的一部分。
// Copyleft 2014 - 2015, LH_Mouse. All wrongs reserved.

#include "../precompiled.hpp"
#include "connection.hpp"
#include "thread_context.hpp"
#include "exception.hpp"
#include "utilities.hpp"
#include <string.h>
#include <stdlib.h>
#include <mysql/mysql.h>
#include "../raii.hpp"
#include "../log.hpp"
#include "../utilities.hpp"

namespace Poseidon {

namespace {
	struct MySqlCloser {
		CONSTEXPR ::MYSQL *operator()() const NOEXCEPT {
			return NULLPTR;
		}
		void operator()(::MYSQL *mysql) const NOEXCEPT {
			::mysql_close(mysql);
		}
	};

	struct MySqlResultDeleter {
		CONSTEXPR ::MYSQL_RES *operator()() const NOEXCEPT {
			return NULLPTR;
		}
		void operator()(::MYSQL_RES *result) const NOEXCEPT {
			::mysql_free_result(result);
		}
	};

	typedef std::vector<std::pair<const char *, std::size_t> > MySqlColumns;

	class MySqlConnectionDelegate : public MySqlConnection {

#define THROW_MYSQL_EXCEPTION	\
	DEBUG_THROW(MySqlException,	\
		::mysql_errno(m_mysql.get()), SharedNts(::mysql_error(m_mysql.get())))

	private:
		// 若返回 true 则 pos 指向找到的列，否则 pos 指向下一个。
		template<typename IteratorT, typename VectorT>
		static bool lowerBoundColumn(IteratorT &pos, VectorT &columns, const char *name){
			AUTO(lower, columns.begin());
			AUTO(upper, columns.end());
			for(;;){
				if(lower == upper){
					pos = lower;
					return false;
				}
				const AUTO(middle, lower + (upper - lower) / 2);
				const int result = ::strcasecmp(middle->first, name);
				if(result == 0){
					pos = middle;
					return true;
				} else if(result < 0){
					upper = middle;
				} else {
					lower = middle + 1;
				}
			}
		}

	private:
		const MySqlThreadContext m_context;

		::MYSQL m_mysqlObject;
		UniqueHandle<MySqlCloser> m_mysql;

		UniqueHandle<MySqlResultDeleter> m_result;
		MySqlColumns m_columns;

		::MYSQL_ROW m_row;
		unsigned long *m_lengths;

	public:
		MySqlConnectionDelegate(const char *serverAddr, unsigned serverPort,
			const char *userName, const char *password, const char *schema,
			bool useSsl, const char *charset)
			: m_row(NULLPTR), m_lengths(NULLPTR)
		{
			if(!m_mysql.reset(::mysql_init(&m_mysqlObject))){
				DEBUG_THROW(SystemError, ENOMEM);
			}

			if(::mysql_options(m_mysql.get(), MYSQL_OPT_COMPRESS, NULLPTR) != 0){
				THROW_MYSQL_EXCEPTION;
			}
			const ::my_bool TRUE_VALUE = true;
			if(::mysql_options(m_mysql.get(), MYSQL_OPT_RECONNECT, &TRUE_VALUE) != 0){
				THROW_MYSQL_EXCEPTION;
			}
			if(::mysql_options(m_mysql.get(), MYSQL_SET_CHARSET_NAME, charset) != 0){
				THROW_MYSQL_EXCEPTION;
			}

			if(!::mysql_real_connect(m_mysql.get(), serverAddr, userName,
				password, schema, serverPort, NULLPTR, useSsl ? CLIENT_SSL : 0))
			{
				THROW_MYSQL_EXCEPTION;
			}
		}

	public:
		void doExecuteSql(const char *sql, std::size_t len){
			m_result.reset();
			m_columns.clear();
			m_row = NULLPTR;

			if(::mysql_real_query(m_mysql.get(), sql, len) != 0){
				THROW_MYSQL_EXCEPTION;
			}

			if(!m_result.reset(::mysql_use_result(m_mysql.get()))){
				if(::mysql_errno(m_mysql.get()) != 0){
					THROW_MYSQL_EXCEPTION;
				}
				// 没有返回结果。
			} else {
				const AUTO(fields, ::mysql_fetch_fields(m_result.get()));
				const AUTO(count, ::mysql_num_fields(m_result.get()));
				m_columns.reserve(count);
				m_columns.clear();
				for(std::size_t i = 0; i < count; ++i){
					MySqlColumns::iterator ins;
					const char *const name = fields[i].name;
					if(lowerBoundColumn(ins, m_columns, name)){
						LOG_POSEIDON_ERROR("Duplicate column in MySQL result set: ", name);
						DEBUG_THROW(Exception, SharedNts::observe("Duplicate column"));
					}
					LOG_POSEIDON_TRACE("MySQL result column: name = ", name, ", index = ", i);
					m_columns.insert(ins, std::make_pair(name, i));
				}
			}
		}

		boost::uint64_t doGetInsertId() const {
			return ::mysql_insert_id(m_mysql.get());
		}

		bool doFetchRow(){
			if(m_columns.empty()){
				LOG_POSEIDON_DEBUG("Empty set returned form MySQL server.");
				return false;
			}
			m_row = ::mysql_fetch_row(m_result.get());
			if(!m_row){
				if(::mysql_errno(m_mysql.get()) != 0){
					THROW_MYSQL_EXCEPTION;
				}
				return false;
			}
			m_lengths = ::mysql_fetch_lengths(m_result.get());
			return true;
		}

		boost::int64_t doGetSigned(const char *column) const {
			MySqlColumns::const_iterator it;
			if(!lowerBoundColumn(it, m_columns, column)){
				LOG_POSEIDON_ERROR("Column not found: ", column);
				DEBUG_THROW(Exception, SharedNts::observe("Column not found"));
			}
			const AUTO(data, m_row[it->second]);
			if(!data || (data[0] == 0)){
				return 0;
			}
			char *endptr;
			const AUTO(val, ::strtoll(data, &endptr, 10));
			if(endptr[0] != 0){
				LOG_POSEIDON_ERROR("Could not convert column data to long long: ", data);
				DEBUG_THROW(Exception, SharedNts::observe("Invalid data format"));
			}
			return val;
		}
		boost::uint64_t doGetUnsigned(const char *column) const {
			MySqlColumns::const_iterator it;
			if(!lowerBoundColumn(it, m_columns, column)){
				LOG_POSEIDON_ERROR("Column not found: ", column);
				DEBUG_THROW(Exception, SharedNts::observe("Column not found"));
			}
			const AUTO(data, m_row[it->second]);
			if(!data || (data[0] == 0)){
				return 0;
			}
			char *endptr;
			const AUTO(val, ::strtoull(data, &endptr, 10));
			if(endptr[0] != 0){
				LOG_POSEIDON_ERROR("Could not convert column data to unsigned long long: ", data);
				DEBUG_THROW(Exception, SharedNts::observe("Invalid data format"));
			}
			return val;
		}
		double doGetDouble(const char *column) const {
			MySqlColumns::const_iterator it;
			if(!lowerBoundColumn(it, m_columns, column)){
				LOG_POSEIDON_ERROR("Column not found: ", column);
				DEBUG_THROW(Exception, SharedNts::observe("Column not found"));
			}
			const AUTO(data, m_row[it->second]);
			if(!data || (data[0] == 0)){
				return 0;
			}
			char *endptr;
			const AUTO(val, ::strtod(data, &endptr));
			if(endptr[0] != 0){
				LOG_POSEIDON_ERROR("Could not convert column data to double: ", data);
				DEBUG_THROW(Exception, SharedNts::observe("Invalid data format"));
			}
			return val;
		}
		std::string doGetString(const char *column) const {
			MySqlColumns::const_iterator it;
			if(!lowerBoundColumn(it, m_columns, column)){
				LOG_POSEIDON_ERROR("Column not found: ", column);
				DEBUG_THROW(Exception, SharedNts::observe("Column not found"));
			}
			std::string val;
			const AUTO(data, m_row[it->second]);
			if(data){
				val.assign(data, m_lengths[it->second]);
			}
			return val;
		}
		boost::uint64_t doGetDateTime(const char *column) const {
			MySqlColumns::const_iterator it;
			if(!lowerBoundColumn(it, m_columns, column)){
				LOG_POSEIDON_ERROR("Column not found: ", column);
				DEBUG_THROW(Exception, SharedNts::observe("Column not found"));
			}
			const AUTO(data, m_row[it->second]);
			if(!data || (data[0] == 0)){
				return 0;
			}
			return scanTime(data);
		}
	};
}

boost::shared_ptr<MySqlConnection> MySqlConnection::create(const char *serverAddr, unsigned serverPort,
	const char *userName, const char *password, const char *schema, bool useSsl, const char *charset)
{
	return boost::make_shared<MySqlConnectionDelegate>(
		serverAddr, serverPort, userName, password, schema, useSsl, charset);
}

MySqlConnection::~MySqlConnection(){
}

void MySqlConnection::executeSql(const char *sql, std::size_t len){
	static_cast<MySqlConnectionDelegate &>(*this).doExecuteSql(sql, len);
}

boost::uint64_t MySqlConnection::getInsertId() const {
	return static_cast<const MySqlConnectionDelegate &>(*this).doGetInsertId();
}
bool MySqlConnection::fetchRow(){
	return static_cast<MySqlConnectionDelegate &>(*this).doFetchRow();
}

boost::int64_t MySqlConnection::getSigned(const char *column) const {
	return static_cast<const MySqlConnectionDelegate &>(*this).doGetSigned(column);
}
boost::uint64_t MySqlConnection::getUnsigned(const char *column) const {
	return static_cast<const MySqlConnectionDelegate &>(*this).doGetUnsigned(column);
}
double MySqlConnection::getDouble(const char *column) const {
	return static_cast<const MySqlConnectionDelegate &>(*this).doGetDouble(column);
}
std::string MySqlConnection::getString(const char *column) const {
	return static_cast<const MySqlConnectionDelegate &>(*this).doGetString(column);
}
boost::uint64_t MySqlConnection::getDateTime(const char *column) const {
	return static_cast<const MySqlConnectionDelegate &>(*this).doGetDateTime(column);
}

}
