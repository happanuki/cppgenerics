#include <vector>
#include <thread>
#include <mutex>

#include "gtest/gtest.h"

#include "Syscalls.h"

class SyscallsTest : public ::testing::Test {
protected:

	void SetUp() override {}

	void TearDown() override {}

};

TEST_F(SyscallsTest,isDirExist)
{
	auto ret_t = System::isDirExist("/tmp");
	auto ret_f = System::isDirExist("/zzzz");
	ASSERT_EQ(ret_t,true);
	ASSERT_EQ(ret_f,false);
}
