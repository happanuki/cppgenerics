#pragma once

#include <exception>
#include <cstring>
#include <cerrno>
#include <string>
#include <sstream>
#include <iostream>

class Exception: public std::exception
{
	std::stringbuf m_buf;
	std::ostream m_stream;
	mutable std::string outStr;

public:
	Exception() : m_stream(&m_buf) {}
	Exception(const Exception& e): m_stream(&m_buf) { m_stream << e.m_buf.str(); }
	Exception(Exception&& e): m_stream(&m_buf) { m_stream << e.m_buf.str(); }

	const char* what() const throw() override {	outStr = m_buf.str(); return outStr.c_str(); }

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
