#include "cppgenerics/FileLock.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"

namespace System {


FileLock::FileLock(int fd):
		m_fd(fd)
{
}

FileLock::~FileLock()
{
	if (m_locker) {
		ERRSTDOUTT("FileLock destructor unlock() ");
		unlock();
	}
}

void FileLock::lock() throw (std::exception&)
{
	m_flock.l_type = F_WRLCK;
	auto ret = fcntl(m_fd,F_SETLKW,&m_flock);
	if (ret < 0) {
		THROW_SYS_EXCEPTION("fnctl F_SETLKW");
	}

	m_locker = true;
}

void FileLock::unlock() noexcept
{
	m_flock.l_type = F_UNLCK;
	(void)fcntl(m_fd,F_SETLKW,&m_flock);

	m_locker = false;
}

} //namespace
