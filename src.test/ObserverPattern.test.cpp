#include <iostream>
#include "gtest/gtest.h"

#include "cppgenerics/ObserverPattern.h"

using ObsBase_t = CppGenerics::ObserverPattern<std::string> ;
using Obs_t = ObsBase_t::Observer_I ;
using Subj_t = ObsBase_t::Subject_I ;

const std::string TEST_STRING("I am test string!");


class TestSubject : public Subj_t
{
public :
	void addObs(Obs_t* o) { Subj_t::addObs(o); }
};

class TestObserver : public Obs_t
{
public:
	void notifyHandler(std::string val) {
		ASSERT_EQ(val, TEST_STRING);
	}
};

using namespace CppGenerics;

class ObserverPatternTest : public ::testing::Test {
protected:

	void SetUp() override {}

	void TearDown() override {}

};

TEST_F(ObserverPatternTest, general_functionality)
{
	TestObserver obs;
	TestSubject subj;

	subj.addObs(&obs);

	subj.notifyObses(TEST_STRING);

	subj.rmObs(&obs);
}
