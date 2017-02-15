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
	if ( m_fileStream.is_open()) {
		m_fileStream.close();
	}
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


void Logger::setLogFile(std::string filename)
{
	if (filename.empty()) {
		return;
	}
	else {
		if ( m_fileStream.is_open() ) {
			if ( m_filename == filename) {
				return;
			}

			m_fileStream.close();
			m_filename = filename;
		}
	}

	m_fileStream.open( filename.c_str(),std::fstream::out | std::fstream::app );
	m_ostream.rdbuf( m_fileStream.rdbuf() );
}
