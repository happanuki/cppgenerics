#pragma once

#include <fcntl.h>

#include <exception>

namespace System {

/*
 * IPC file locker, PID locker, not thread one
 */
class FileLock
{
	int m_fd = -1;
	bool m_locker = false;
	flock m_flock = { 0 };

public:
	FileLock() = delete;
	FileLock& operator=(const FileLock& o) = delete;
	FileLock(const FileLock& o) = delete;
	FileLock(FileLock&& o) = delete;

	FileLock(int fd);
	~FileLock();

	void lock() throw (std::exception&);
	void unlock() noexcept;
};

} //end namespace
