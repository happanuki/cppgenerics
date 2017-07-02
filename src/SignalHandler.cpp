#include <thread>
#include <algorithm>
#include <functional>
#include <map>

#include "cppgenerics/SignalHandler.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"
#include "cppgenerics/Syscalls.h"

namespace CppGenerics {

namespace Signals {

//using _handler_t = void(*)(int, siginfo_t*, void*);

static std::vector<SignalDescriptor> sg_signals; // (static) global signal table
static std::mutex sg_signalsMtx;

thread_local static sigset_t tl_ss;


void SignalHandler::_init() throw (std::exception&)
{
	(void) sigfillset(&m_ss);

	auto ret = pthread_sigmask(SIG_SETMASK, &m_ss, nullptr);
	if ( ret ) {
		THROW_SYS_EXCEPTION("pthread_sigmask() failed");
	}

	(void) sigemptyset(&m_ss);
}

SignalHandler::SignalHandler() :
		g_signals(sg_signals),
		g_signalsMtx(sg_signalsMtx),
		m_ss(tl_ss)
{
	_init();
}


thread_local static std::map<_sig_t, decltype(SignalDescriptor::handler) > tl_handlers;

SignalHandler::~SignalHandler()
{
	DEBUGSTDOUTT(" call ")

	for (auto& it: tl_handlers) {
		rmSignal(it.first);
	}

	tl_handlers.clear();
	_init();
}


static void c_sigHandler(int sigNum, siginfo_t* info, void* prevH) noexcept
{
	auto cpp_handler = tl_handlers[sigNum];
	cpp_handler(sigNum,info,prevH);
}


void SignalHandler::addSignal(const SignalDescriptor& d) throw (std::exception&)
{
	std::lock_guard<std::mutex> g( g_signalsMtx);

	auto found = std::find_if( g_signals.begin(), g_signals.end(), [&d](const SignalDescriptor& in) { return d.sigNum == in.sigNum; });
	if ( found != g_signals.end() ) {
		THROW_LOGIC_EXCEPTION("signal : " << d.sigNum << " already in sg_table");
	}

	//adding to thread-local and global containers

	tl_handlers[d.sigNum] = d.handler ;
	g_signals.push_back(d);

	//Installing handler

	sigset_t l_ss;
	(void) sigfillset( &l_ss);

	int flags = SA_SIGINFO ;

	if ( ! d.isDeffered ) {
		(void) sigdelset( &l_ss,d.sigNum);
		flags |= SA_NODEFER;
	}

	struct sigaction act;
	act.sa_flags = flags;
	act.sa_sigaction = c_sigHandler;
	act.sa_mask = l_ss;
	act.sa_restorer = nullptr;

	auto ret = sigaction(d.sigNum, &act, nullptr);
	if ( ret) {
		THROW_SYS_EXCEPTION("sigaction() failed!");
	}

	//Subscribing for signals

	(void) sigaddset( &m_ss,d.sigNum);

	ret = pthread_sigmask( SIG_UNBLOCK, &m_ss, nullptr);
	if ( ret ) {
		THROW_SYS_EXCEPTION("pthread_sigmask() failed");
	}

}



void SignalHandler::rmSignal(_sig_t sigNum)
{
	DEBUGSTDOUTT(" start rm sigNum : " << sigNum);
	sigset_t l_ss;
	(void) sigemptyset(&l_ss);
	(void) sigaddset(&l_ss, sigNum);

	auto ret = pthread_sigmask( SIG_BLOCK, &l_ss, nullptr);
	if ( ret ) {
		THROW_SYS_EXCEPTION("pthread_sigmask() failed");
	}

	{
		std::lock_guard<std::mutex> g(g_signalsMtx);

		auto it = std::find_if( g_signals.begin(), g_signals.end(),
				[sigNum](const SignalDescriptor& d)
				{
					if (d.sigNum == sigNum) DEBUGSTDOUTT("found sigNum" << sigNum) ;
					return d.sigNum == sigNum ;
				}
		);

		g_signals.erase(it);

	}

	tl_handlers.erase(sigNum);

	(void) sigdelset( &m_ss,sigNum);

	ret = pthread_sigmask( SIG_UNBLOCK, &m_ss, nullptr);
	if ( ret ) {
		THROW_SYS_EXCEPTION("pthread_sigmask() failed");
	}

}


} //namespace

} // namespace CppGenerics
