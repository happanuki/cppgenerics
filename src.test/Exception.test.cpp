#include <vector>
#include <thread>
#include <mutex>

#include "gtest/gtest.h"

#include "cppgenerics/Exception.h"

class ExceptionTest : public ::testing::Test {
protected:

	void SetUp() override {
	}

	void TearDown() override {}

};

//Should be first, singletone
TEST_F(ExceptionTest, API_test)
{
	std::string errText = "Some err text";

	Exception e;
	e() << errText;

	EXPECT_STREQ(errText.c_str(),e.what());

}

TEST_F(ExceptionTest, MACRO_test)
{
	std::string errText = "Some err text";

	EXPECT_THROW(THROW_LOGIC_EXCEPTION(errText),Exception);

}


TEST_F(ExceptionTest, CopyTest)
{
	std::string errText = "Some err text";

	Exception e;
	e() << errText;

	Exception e2(e);

	EXPECT_STREQ(errText.c_str(),e2.what());

}
