#pragma once

#include "cppgenerics/IPC_Buffer.h"

namespace IPC
{

class MutexManager
{
	struct MutexDescriptor
	{
		futex_id_t m_ftx = 0;
		char m_id[IPC_MUTEXMANAGER_DESCRIPTOR_MAX_LEN + 1] = { 0 };
	};

	size_t m_capacity = IPC_MUTEXMANAGER_CAPACITY ;
	std::string m_bufId = "MutexManager-buffer" ;
	Buffer m_buf;

	MutexDescriptor* findDescriptor(const std::string& id) noexcept;
	MutexDescriptor* findFreeDescriptor() noexcept;
	void initDescriptor(MutexDescriptor* desc, const std::string& id);

	void init();
	MutexManager() {}

public:
	static MutexManager& getInstance() throw (std::exception&);

	MutexManager(const MutexManager&) = delete;
	MutexManager(MutexManager&&) = delete;
	MutexManager& operator=(MutexManager&) = delete;
	~MutexManager() {}

	Mutex getMutex(const std::string& id) throw (std::exception&);
};

} //end namespace
