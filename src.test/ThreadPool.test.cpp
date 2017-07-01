#include "gtest/gtest.h"

#include "cppgenerics/ThreadPool.h"
#include <iostream>

class ThreadPoolTest : public ::testing::Test {

protected:

	void SetUp() override {}
	void TearDown() override {}
};


TEST_F(ThreadPoolTest, general_functionality)
{
	using Arg_t = struct
	{
		std::string str = "TestString!";
		int num = -1;
	};
	auto f = [](const Arg_t& val) {
		static std::mutex sm;
		std::lock_guard< std::mutex> g(sm);
		std::cout << "STR : " << val.str << "\t\tNUM: " << val.num << std::endl;
	};

	ThreadPool::ThreadPool<Arg_t> t(f,2);

	for (auto i = 0; i < 5; ++i) {

		std::string str = "xxxTest" + std::to_string(i) + "xxx";
		Arg_t val;
		val.str = str;
		val.num = i;

		t.getThread(val).detach();
	}

}
