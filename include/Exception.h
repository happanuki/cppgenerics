#pragma once

#include <exception>
#include <cstring>
#include <cerrno>
#include <string>
#include <sstream>

class Exception: public std::exception
{
	std::ostringstream m_stream;

public:
	Exception() = default;
	Exception(const Exception& e) { m_stream << e.m_stream.str() ; }

	Exception(std::string&& str) { m_stream << str; }

	const char* what() const throw() override {	return m_stream.str().c_str(); }

	std::ostream& operator()() { return m_stream; }
};


#define THROW_SYS_EXCEPTION( MSG ) \
			{ \
				Exception e; \
				e() << "[ " <<  __FILE__  << ":" << __LINE__ << " @ \"" << __PRETTY_FUNCTION__ << "\" ] : " << MSG << \
				"\n\tERRNO : " << errno << "\n\tERRMSG: " << strerror(errno) ; \
				throw e; \
			}

#define THROW_LOGIC_EXCEPTION( MSG ) \
			{ \
				Exception e; \
				e() << "[ " <<  __FILE__  << ":" << __LINE__ << " @ \"" << __PRETTY_FUNCTION__ << "\" ] : " << MSG ; \
				throw e; \
			}
