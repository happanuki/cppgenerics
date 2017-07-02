#include "gtest/gtest.h"

#include <mutex>

#include "cppgenerics/FileLock.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Syscalls.h"


using namespace CppGenerics;

class FileLockTest : public ::testing::Test {
protected:
	int m_badFd = -5;
	std::string m_goodFname = "/tmp/fileLockTest.tst";

	void SetUp() override {}

	void TearDown() override {}

};

TEST_F(FileLockTest,testGoodDescriptor_MT)
{
	const int processNum = 20;

	for (int i= 0; i < processNum; ++i ) {
		int pid = ::fork();
		if (!pid) {
			//child
			auto m_goodFd = System::openAutoClose( m_goodFname.c_str(), O_RDWR|O_CREAT, S_IRUSR|S_IWUSR);

			System::FileLock fl( *m_goodFd);
			std::lock_guard<System::FileLock> g( fl);

			std::string inStr = "Process " + std::to_string(i) + " test write";

			System::write( *m_goodFd,inStr.c_str(),inStr.size());
			::fsync( *m_goodFd);

			System::lseek( *m_goodFd,0,SEEK_SET);

			auto sz = sysconf(_SC_PAGESIZE);
			auto buf = System::getBufferRAI( sz, true);
			System::read( *m_goodFd, buf.get(), sz );

			std::string outStr( buf.get());

			ASSERT_EQ(inStr,outStr);

			truncate(m_goodFname.c_str(),0);

			::exit(0);
		}
		else {
			//parent
		}
	}

	wait(nullptr);
}

TEST_F(FileLockTest,testBadDescriptor_MT)
{
	System::FileLock fl(m_badFd);
	ASSERT_THROW( fl.lock(), Exception);
}

