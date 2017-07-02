#pragma once

#include <signal.h>

#include <exception>
#include <vector>
#include <mutex>
#include <thread>
#include "cppgenerics/Logger.h"


namespace CppGenerics {

namespace Signals {

using _sig_t = int;

struct SignalDescriptor
{
	typename std::thread::id tid;
	_sig_t sigNum = -1;
	bool isDeffered = true;
	std::function<void (int ,siginfo_t*,void*)> handler;

	void dumpMe() {
		LOGSTDOUT(	"\t\t----- SIGNAL DESCRIPTOR DUMP--------" <<
					"\n" << "\tTID :\t\t" << tid <<
					"\n" << "\tisDeffered :\t" << isDeffered <<
					"\n" << "------------------------------------" <<
					std::endl );
	}

	SignalDescriptor(_sig_t sigNum, decltype(handler) handler, bool isDeffered = false) :
		sigNum(sigNum),
		handler(handler),
		isDeffered(isDeffered)
	{
		tid = std::this_thread::get_id();
	}

	SignalDescriptor() = default;
	SignalDescriptor(const SignalDescriptor&) = default;
	SignalDescriptor(SignalDescriptor&&) = default;
	SignalDescriptor& operator=(const SignalDescriptor&) = default;

};


class SignalHandler
{
	sigset_t& m_ss;

	std::vector<SignalDescriptor>& g_signals; //ref to global app signal table
	std::mutex& g_signalsMtx;

	void _init() throw (std::exception&);

public:
	SignalHandler(); //may throw std::exception&
	~SignalHandler();

	void addSignal(const SignalDescriptor& d) throw (std::exception&);
	void rmSignal(_sig_t sigNum);
};

} // namespace Signals

} // namespace CppGenerics
