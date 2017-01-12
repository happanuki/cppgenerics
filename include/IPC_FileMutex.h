#pragma once

#include "IPC_Mutex.h"

namespace IPC
{

class FileMutex {

	//Init as file parameters
	std::string m_filename;
	int m_sharedMemKey = -1;

	Mutex* m_mtx;

	futex_id_t* initFutexStorage(int sharedMemKey, bool isNew = false) throw (std::exception&);
	void initUsingFile() throw (std::exception&);

public:
	FileMutex();
	FileMutex(const FileMutex&);
	~FileMutex();
	FileMutex& operator=(const FileMutex& o) = delete;
	FileMutex(std::string&& filename);

	inline void lock() throw (std::exception&) { m_mtx->lock(); }
	inline void unlock() noexcept { m_mtx->unlock(); }
};

} //namespace
