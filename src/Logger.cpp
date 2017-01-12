#include <iostream>
#include <memory>

#include "Logger.h"

static std::unique_ptr<Logger> loggerHolder;

Logger::Logger() :
		m_ostream( std::cout.rdbuf() )
{
}

Logger::~Logger()
{
}

Logger& Logger::getInstance() throw (std::exception&)
{
	static std::mutex s_mtx;
	std::lock_guard<std::mutex> g( s_mtx);

	if (loggerHolder.get() == nullptr) {

		(void) atexit( [](){ loggerHolder.reset(); } );

		loggerHolder.reset(new Logger());
	}

	return *loggerHolder;
}


void Logger::setLogSTDOUT()
{
	m_ostream.rdbuf( std::cout.rdbuf() );
}


void Logger::setLogSTDERR()
{
	m_ostream.rdbuf( std::cerr.rdbuf() );
}


std::ostream& Logger::log()
{
	return m_ostream;
}
