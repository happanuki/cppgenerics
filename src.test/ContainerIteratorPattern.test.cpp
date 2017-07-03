#include <iostream>
#include "gtest/gtest.h"
#include <limits>

#include "cppgenerics/ContainerIteratorPattern.h"
#include "cppgenerics/Logger.h"

const std::string TEST_STRING("I am test string!");

using ContIterBase_t = CppGenerics::ContainerIteratorPattern<std::string> ;
using Iterator = ContIterBase_t::Iterator;
using Idx_t = ContIterBase_t::Idx_t;

class TestContainer : public ContIterBase_t::Container
{
	std::string m_val;
	Iterator _getIterator(Idx_t index) override { return Iterator(this, index); }
	Idx_t _nextIdx(Idx_t index) override { return ++index; }
	Idx_t _prevIdx(Idx_t index) override { return --index; }

	std::string& _getValue( Idx_t index ) override { m_val = TEST_STRING + std::to_string( index); return m_val; }

public :
	Iterator begin() { return Iterator(this,0); }
	Iterator end() { return Iterator(this,15); }
};


class ContainerIteratorPatternTest: public ::testing::Test {
protected:

	void SetUp() override {}

	void TearDown() override {}

};

TEST_F(ContainerIteratorPatternTest, general_functionality)
{
	TestContainer cont;

	int testCnt = 0;

	for (auto it = cont.begin(); it != cont.end(); ++it ) {
		ASSERT_EQ(*it, std::string(TEST_STRING + std::to_string( testCnt)));
		++testCnt;
	}


}
