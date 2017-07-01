#pragma once

#include <exception>
#include <string>
#include <ostream>
#include <mutex>
#include <fstream>

#include "cppgenerics/Syscalls.h"

class Logger {

	std::ostream m_ostream;
	std::mutex m_ostreamMtx;

	std::string m_filename;
	std::fstream m_fileStream;

	Logger();

public:
	~Logger();
	Logger(const Logger&) = delete;
	Logger& operator=(const Logger&) = delete;

	static Logger& getInstance() throw (std::exception&);

	void setLogSTDOUT();
	void setLogSTDERR();
	void setLogFile(std::string filename = {});

	std::ostream& log();

	inline void lock() { m_ostreamMtx.lock(); }
	inline void unlock() noexcept { m_ostreamMtx.unlock(); }

};

#ifdef DEBUG
#define LOGSTDOUT(MSG) \
		{ \
			std::lock_guard<Logger> g( Logger::getInstance() ); \
			Logger::getInstance().setLogSTDOUT() ; \
			Logger::getInstance().log() << \
				"[ " __FILE__ << ":"  << __LINE__ << " @ \"" << __PRETTY_FUNCTION__ << "\" ] :\t" << MSG << std::endl ; \
		}

#define LOGSTDOUTT(MSG) \
		{ \
			std::lock_guard<Logger> g( Logger::getInstance() ); \
			Logger::getInstance().setLogSTDOUT() ; \
			Logger::getInstance().log() << "<" << System::timeStr(nullptr) << "> " \
				"[ " __FILE__ << ":"  << __LINE__ << " @ \"" << __PRETTY_FUNCTION__ << "\" ] :\t" << MSG << std::endl ; \
		}


#define DEBUGSTDOUT(DMSG) LOGSTDOUT( "**DEBUG** \t" << DMSG )
#define DEBUGSTDOUTT(DMSG) LOGSTDOUTT( "**DEBUG** \t" << DMSG )
#else
#define LOGSTDOUT(MSG) \
		{ \
			std::lock_guard<Logger> g( Logger::getInstance() ); \
			Logger::getInstance().setLogSTDOUT() ; \
			Logger::getInstance().log() << MSG << std::endl ; \
		}

#define LOGSTDOUTT(MSG) \
		{ \
			std::lock_guard<Logger> g( Logger::getInstance() ); \
			Logger::getInstance().setLogSTDOUT() ; \
			Logger::getInstance().log() << "<" << System::timeStr(nullptr) << "> " << MSG << std::endl ; \
		}

#define LOGSTDOUTT(MSG) \
		{ \
			std::lock_guard<Logger> g( Logger::getInstance() ); \
			Logger::getInstance().setLogSTDOUT() ; \
			Logger::getInstance().log() << "<" << System::timeStr(nullptr) << "> " << MSG << std::endl ; \
		}

#define DEBUGSTDOUT(DMSG)
#define DEBUGSTDOUTT(DMSG)
#endif

#define ERRSTDOUT(DMSG) LOGSTDOUT( "** ERROR ** \t" << DMSG )
#define ERRSTDOUTT(DMSG) LOGSTDOUTT( "** ERROR ** \t" << DMSG )

#define WARNSTDOUT(DMSG) LOGSTDOUT( "** WARN ** \t" << DMSG )
#define WARNSTDOUTT(DMSG) LOGSTDOUTT( "** WARN ** \t" << DMSG )

#define LOGINFILE(MSG,...) \
		{ \
			std::lock_guard<Logger> g( Logger::getInstance() ); \
			Logger::getInstance().setLogFile(__VA_ARGS__) ; \
			Logger::getInstance().log() << MSG << std::endl ; \
		}
