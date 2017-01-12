#include "fcntl.h"
#include "unistd.h"
#include <sys/ipc.h>
#include <sys/shm.h>

#include <mutex>

#include "IPC_FileMutex.h"
#include "FileLock.h"
#include "Syscalls.h"
#include "Exception.h"
#include "Logger.h"


namespace IPC
{

/*
 * FileMutex
 */


futex_id_t* FileMutex::initFutexStorage(int sharedMemKey, bool isNew) throw (std::exception&)
{
	int l_flags;
	isNew == true ? l_flags = IPC_CREAT|IPC_EXCL| SHM_NORESERVE| S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP : l_flags = 0;

	auto shmId = shmget(sharedMemKey, sizeof(futex_id_t), l_flags);
	if ( shmId < 0) {
		THROW_SYS_EXCEPTION("shmget failed");
	}

	auto ret = shmat(shmId,nullptr,0);
	if ( ret == (void*)-1) {
		THROW_SYS_EXCEPTION("shmat failed");
	}

	return (futex_id_t*)ret;
}
//
void FileMutex::initUsingFile() throw (std::exception&)
{

	auto fd = System::openAutoClose(m_filename.c_str(),O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

	System::FileLock fdLock(*fd);
	std::lock_guard<System::FileLock> g(fdLock);

	auto bytes = System::read(*fd,&m_sharedMemKey,sizeof(m_sharedMemKey));
	if ( bytes == 0 ) {
		//creating new futex
		m_sharedMemKey = System::getRandom(0x100,INT32_MAX);

		auto futexPtr = initFutexStorage(m_sharedMemKey,true);
		m_mtx = new Mutex(futexPtr,true);

		System::write(*fd, &m_sharedMemKey, sizeof(m_sharedMemKey));
	}
	else
	{
		auto futexPtr = initFutexStorage(m_sharedMemKey);
		m_mtx = new Mutex(futexPtr,false);
	}

}

FileMutex::~FileMutex()
{
	if ( m_sharedMemKey != -1 ) {
		auto ptr = m_mtx->getPtr();

		delete m_mtx;

		(void)::shmdt(ptr);
	}
}


FileMutex::FileMutex(std::string&& filename) :
	m_filename(filename)
{
	initUsingFile();
}


} //namespace
