#pragma once

#include <thread>
#include <functional>
#include <mutex>
#include <condition_variable>

#include "cppgenerics/Logger.h"

namespace CppGenerics {

/*
 * returns std::thread< void(const T& arg)>
 */

template < typename T > class ThreadPool
{
public:
	using ThreadFunctionArg_t = T ;
	using ThreadFunction_t = std::function< void(const T&)> ;

	explicit ThreadPool(ThreadFunction_t func, size_t numThreads);
	~ThreadPool();

	std::thread getThread(const T& arg);

private:
	size_t m_maxThreads;
	size_t m_curThreads = 0;

	std::mutex m_getPutMtx;
	std::condition_variable m_getPutCV;
	ThreadFunction_t m_fun;

	void _getThread();
	void _putThread();
	void _waitAll() noexcept;
	void _validateArgs() noexcept;

	void _threadF(const T&);
};



// Template implementation

template < typename T> ThreadPool<T>::ThreadPool(ThreadFunction_t func, size_t numThreads) :
	m_maxThreads(numThreads),
	m_fun(func)
{
	_validateArgs();
}


template < typename T> void ThreadPool<T>::_validateArgs() noexcept
{
	if ( m_maxThreads == 0) {
		WARNSTDOUTT("Trying to create ThreadPool with 0 threads, creating with 1 instead");
		m_maxThreads = 1;
	}
}


template < typename T> ThreadPool<T>::~ThreadPool()
{
	_waitAll();
}


template < typename T> void ThreadPool<T>::_waitAll() noexcept
{
	try {

		std::unique_lock<std::mutex> l(m_getPutMtx);

		while ( m_curThreads ) {
			m_getPutCV.wait( l );
		}
	}
	catch ( std::exception& e ) {
		ERRSTDOUTT(" ThreadPool _waitAll() exception : " << e.what() )
	}
}

template < typename T> void ThreadPool<T>::_getThread()
{
	std::unique_lock<std::mutex> l(m_getPutMtx);

	while ( m_curThreads >= m_maxThreads ) {
		m_getPutCV.wait( l );
	}

	++m_curThreads;
}


template < typename T> void ThreadPool<T>::_putThread()
{
	{
		std::lock_guard< std::mutex> g( m_getPutMtx);

		--m_curThreads;
	}

	m_getPutCV.notify_one();
}


template < typename T> void ThreadPool<T>::_threadF(const T& arg)
{
	m_fun(arg);
	_putThread();
}

template < typename T> std::thread ThreadPool<T>::getThread(const T& arg)
{
	using namespace std::placeholders;

	_getThread();

	auto funcAdapter = std::bind( &ThreadPool::_threadF, this, _1 );

	std::thread out( funcAdapter, arg);
	return out;
}


} // namespace CppGenerics
