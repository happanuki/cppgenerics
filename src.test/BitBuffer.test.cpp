#include <malloc.h>

#include <vector>
#include <thread>
#include <mutex>

#include <memory>

#include "gtest/gtest.h"

#include "cppgenerics/BitBuffer.h"
#include "cppgenerics/Syscalls.h"
#include "cppgenerics/Logger.h"
#include "cppgenerics/Exception.h"

using namespace CppGenerics;

class BitBufferTest : public ::testing::Test {

protected:

	std::unique_ptr<BitBuffer> buf;
	size_t elems = 100000;

	void SetUp() override {}

	void TearDown() override {}
public:
	BitBufferTest()
	{
		buf.reset( new BitBuffer(elems));
	}

	~BitBufferTest() {}
};

TEST_F(BitBufferTest, self_init_0xFF)
{
	buf->init();

	auto bytes = BitBuffer::getSizeBytes(elems);
	auto ptr = buf->getInternalStorage();
	bool initOk = true;

	uint8_t mask = BitBuffer::FREE_BIT ? 0xFF : 0x00 ;

	for (auto i = 0; i < bytes; ++i) {
		if ( ptr[i] != mask ) {
			initOk = false;
			break;
		}
	}
	ASSERT_EQ(initOk, true);
}

TEST_F(BitBufferTest, getter_setter_correct)
{
	buf->init();

	auto idx = 500;
	buf->set(idx,BitBuffer::FULL_BIT);

	auto res = buf->get(idx);

	ASSERT_EQ(res,BitBuffer::FULL_BIT);
}

TEST_F(BitBufferTest, getter_setter_incorrect)
{
	buf->init();

	auto idx = elems+1;
	ASSERT_THROW( buf->set(idx,BitBuffer::FULL_BIT), Exception) ;

	ASSERT_THROW( buf->get(idx), Exception) ;
}


TEST_F(BitBufferTest, find)
{
	BitBuffer b(10);
	b.init();
	b.set(3,BitBuffer::FULL_BIT); // [0..2], [4..9]

	EXPECT_EQ( b.find(3), 0);
	EXPECT_EQ( b.find(6), 4);
	EXPECT_EQ( b.find(5), 4);
	EXPECT_EQ( b.find(7), -1);
}
