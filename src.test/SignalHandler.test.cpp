#include <unistd.h>

#include <thread>
#include <mutex>
#include <memory>
#include <functional>

#include "gtest/gtest.h"

#include "cppgenerics/SignalHandler.h"
#include "cppgenerics/Logger.h"

using namespace CppGenerics;

class SignalHandlerTest : public ::testing::Test {
protected:
	void SetUp() override {}

	void TearDown() override {}
};


TEST_F(SignalHandlerTest, Create_instance)
{
	std::unique_ptr<Signals::SignalHandler> h ( new Signals::SignalHandler() );
	ASSERT_NE( h.get(),nullptr );
}


static int testPid = 0;
static bool shouldStop = false;
static void sigFuncS(int num, siginfo_t* info, void* oldH)
{
//	LOGSTDOUTT("signum: " << num << " info: " << info->si_pid )
	EXPECT_EQ(num, SIGURG);
	EXPECT_EQ(info->si_signo, SIGURG);
	EXPECT_EQ(info->si_pid, testPid);
	shouldStop = true;
}


TEST_F(SignalHandlerTest, static_void_handler_and_SignalDescriptor_constructor)
{
	using namespace Signals;
	using namespace std::placeholders ;

	shouldStop = false;
	auto pid = getpid();

	std::thread sigGeneratorThread( [pid]() {
		sleep(0);
		testPid = getpid();
		kill(pid,SIGURG);
	});

	std::thread sigHandlerThread( []() {

		SignalHandler h;
		SignalDescriptor d(SIGURG,sigFuncS);

		h.addSignal(d);
		while (! shouldStop) {
			sleep(0);
		}

	});

	sigGeneratorThread.join();
	sigHandlerThread.join();
}


TEST_F(SignalHandlerTest, lambda_handler)
{
	using namespace Signals;
	using namespace std::placeholders ;

	shouldStop = false;
	auto pid = getpid();

	std::thread sigGeneratorThread( [pid]() {
		sleep(0);
		testPid = getpid();
		kill(pid,SIGURG);
	});

	std::thread sigHandlerThread( []() {

		SignalHandler h;
		SignalDescriptor d;
		d.tid = std::this_thread::get_id();
		d.sigNum = SIGURG;
		d.isDeffered = true;
		d.handler =  [&d](int num, siginfo_t* info, void* oldH)
				{
//					LOGSTDOUTT("signum: " << num << " info: " << info->si_pid )
					EXPECT_EQ(num, SIGURG);
					EXPECT_EQ(info->si_signo, SIGURG);
					EXPECT_EQ(info->si_pid, testPid);
					EXPECT_EQ(d.tid, std::this_thread::get_id());
//					d.dumpMe();
					shouldStop = true;
				};

		h.addSignal(d);
		while (! shouldStop) {
			sleep(0);
		}

	});

	sigGeneratorThread.join();
	sigHandlerThread.join();
}


TEST_F(SignalHandlerTest, Single_handler_multiple_thread_test)
{
	using namespace Signals;
	using namespace std::placeholders ;

	shouldStop = false;
	auto pid = getpid();

	std::thread sigGeneratorThread( [pid]() {
		sleep(0);
		testPid = getpid();
		kill(pid,SIGURG);
		kill(pid,SIGWINCH);
	});

	std::thread sigHandlerThread_one( []() {
		bool l_shouldStop = false;

		SignalHandler h;
		SignalDescriptor d;
		d.tid = std::this_thread::get_id();
		d.sigNum = SIGURG;
		d.isDeffered = true;
		d.handler =  [&d, &l_shouldStop](int num, siginfo_t* info, void* oldH)
				{
//					LOGSTDOUTT("signum: " << num << " info: " << info->si_pid )
					EXPECT_EQ(num, SIGURG);
					EXPECT_EQ(info->si_signo, SIGURG);
					EXPECT_EQ(info->si_pid, testPid);
					EXPECT_EQ(d.tid, std::this_thread::get_id());
					l_shouldStop = true;
				};

		h.addSignal(d);
		while (! l_shouldStop) {
			sleep(0);
		}

	});

	std::thread sigHandlerThread_two( []() {

		bool l_shouldStop = false;
		SignalHandler h;
		SignalDescriptor d;
		d.tid = std::this_thread::get_id();
		d.sigNum = SIGWINCH;
		d.isDeffered = true;
		d.handler =  [&d, &l_shouldStop](int num, siginfo_t* info, void* oldH)
				{
//					LOGSTDOUTT("signum: " << num << " info: " << info->si_pid )
					EXPECT_EQ(num, SIGWINCH);
					EXPECT_EQ(info->si_signo, SIGWINCH);
					EXPECT_EQ(info->si_pid, testPid);
					EXPECT_EQ(d.tid, std::this_thread::get_id());
					l_shouldStop = true;
				};

		h.addSignal(d);
		while (! l_shouldStop) {
			sleep(0);
		}

	});

	sigGeneratorThread.join();
	sigHandlerThread_one.join();
	sigHandlerThread_two.join();
}


TEST_F(SignalHandlerTest, Multiple_handlers_single_thread_test)
{
	using namespace Signals;
	using namespace std::placeholders ;

	shouldStop = false;
	auto pid = getpid();

	std::thread sigGeneratorThread( [pid]() {
		sleep(0);
		testPid = getpid();
		kill(pid,SIGURG);
		kill(pid,SIGWINCH);
	});

	std::thread sigHandlerThread_one( []() {
		bool l_shouldStop_one = false;
		bool l_shouldStop_two = false;

		SignalHandler h;
		SignalDescriptor d;
		d.tid = std::this_thread::get_id();
		d.sigNum = SIGURG;
		d.isDeffered = true;
		d.handler =  [&d, &l_shouldStop_one](int num, siginfo_t* info, void* oldH)
				{
//					LOGSTDOUTT("signum: " << num << " info: " << info->si_pid )
					EXPECT_EQ(num, SIGURG);
					EXPECT_EQ(info->si_signo, SIGURG);
					EXPECT_EQ(info->si_pid, testPid);
					EXPECT_EQ(d.tid, std::this_thread::get_id());
					l_shouldStop_one = true;
				};

		h.addSignal(d);

		SignalDescriptor d1;
		d1.tid = std::this_thread::get_id();
		d1.sigNum = SIGWINCH;
		d1.isDeffered = true;
		d1.handler =  [&d1, &l_shouldStop_two](int num, siginfo_t* info, void* oldH)
				{
//					LOGSTDOUTT("signum: " << num << " info: " << info->si_pid )
					EXPECT_EQ(num, SIGWINCH);
					EXPECT_EQ(info->si_signo, SIGWINCH);
					EXPECT_EQ(info->si_pid, testPid);
					EXPECT_EQ(d1.tid, std::this_thread::get_id());
					l_shouldStop_two = true;
				};

		h.addSignal(d1);

		while (! l_shouldStop_two) {
			sleep(0);
		}

	});

	sigGeneratorThread.join();
	sigHandlerThread_one.join();
}

