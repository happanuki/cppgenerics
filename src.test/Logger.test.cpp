#include <vector>
#include <thread>
#include <mutex>
#include <algorithm>
#include <fstream>

#include "gtest/gtest.h"

#include "cppgenerics/Logger.h"


using namespace CppGenerics;

class LoggerTest : public ::testing::Test {
protected:
	Logger* m_logger = nullptr;

	void SetUp() override {
	}

	void TearDown() override {}

};

//Should be first, singletone
TEST_F(LoggerTest,concurrentGetInstance)
{
	std::vector<unsigned long> addrs;
	std::mutex v_mtx;
	std::vector<std::thread> threads;

	const int threadsNum = 20;

	for (int i= 0; i < threadsNum; ++i ) {
		threads.emplace_back( std::thread([&addrs,&v_mtx](){
			Logger* addr = &Logger::getInstance();
			std::lock_guard<std::mutex> g( v_mtx);
			addrs.push_back((unsigned long) addr);
		}));
	}

	for (auto& t: threads) {
		t.join();
	}

	auto eta = *addrs.begin();

	for (auto it: addrs) {
		ASSERT_EQ(it,eta);
	}
}

TEST_F(LoggerTest,checkLoggerMacroses)
{
	std::string testString = "HelloWorld!123456\n\n";

	testing::internal::CaptureStdout();

	LOGSTDOUT(testString);

	std::string output = testing::internal::GetCapturedStdout();
	EXPECT_EQ(testString + "\n", output);
}

TEST_F(LoggerTest,checkLoggerOutputOstream)
{
	std::string testString = "HelloWorld!123456\n\n";

	testing::internal::CaptureStdout();

	Logger::getInstance().setLogSTDOUT();
	Logger::getInstance().log() << testString ;

	std::string output = testing::internal::GetCapturedStdout();
	EXPECT_EQ(testString, output);
}


TEST_F(LoggerTest,checkLoggerOutputFile)
{
	std::string file("/tmp/testLogger.file");

	unlink(file.c_str());

	std::vector<std::string> v = { "Hello your world", "World!","You" };

	for (auto& i: v) {
		LOGINFILE(i, file);
	}

	std::fstream fs(file, std::fstream::in);

	std::string out;
	int idx=0;

	while ( std::getline(fs,out,'\n')) {

		auto v1 = out;
		auto v2 = v[idx++];

		ASSERT_STREQ( v1.c_str(), v2.c_str() );
	}

}

