#include "cppgenerics/IPC_MutexManager.h"

#include "cppgenerics/Syscalls.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"


namespace CppGenerics {

namespace IPC {

/*
 * MutexManager
 */

std::unique_ptr<MutexManager> mm_instanceHolder(nullptr);

MutexManager& MutexManager::getInstance() throw (std::exception&)
{
	static std::mutex s_mtx;
	std::lock_guard<std::mutex> g(s_mtx);

	if (mm_instanceHolder == nullptr) {

		(void) atexit( [](){ mm_instanceHolder.reset(); } );

		mm_instanceHolder.reset( new MutexManager() );
		if (mm_instanceHolder == nullptr) {
			THROW_SYS_EXCEPTION("malloc failed");
		}

		mm_instanceHolder->init();
	}

	return *mm_instanceHolder;
}


void MutexManager::init()
{
	auto calcBytes = sizeof(MutexDescriptor) * m_capacity;
	m_buf = BufferManager::getInstance().getBuffer(m_bufId, calcBytes);
}


MutexManager::MutexDescriptor* MutexManager::findDescriptor(const std::string& id) noexcept
{
	MutexDescriptor* b = static_cast<MutexDescriptor*>( m_buf.getPtr());
	for ( auto i=0; i < m_capacity; ++i) {
		if (! strncmp(id.c_str(), b[i].m_id, IPC_MUTEXMANAGER_DESCRIPTOR_MAX_LEN) ) {
			return &b[i];
		}
	}
	return nullptr;
}


MutexManager::MutexDescriptor* MutexManager::findFreeDescriptor() noexcept
{
	MutexDescriptor* b = static_cast<MutexDescriptor*>( m_buf.getPtr());
	for ( auto i=0; i < m_capacity; ++i) {
		if (! b[i].m_id[0] ) {
			return &b[i];
		}
	}
	return nullptr;
}


void MutexManager::initDescriptor(MutexDescriptor* desc,const std::string& id)
{
	strncpy(desc->m_id,id.c_str(),IPC_MUTEXMANAGER_DESCRIPTOR_MAX_LEN);
}


Mutex MutexManager::getMutex(const std::string& id) throw (std::exception&)
{
	auto ptr = findDescriptor(id);
	if ( ptr == nullptr ) {

		ptr = findFreeDescriptor();
		if (ptr == nullptr) {
			THROW_LOGIC_EXCEPTION("Cant alloc more Mutexes");
		}
		else {
			initDescriptor(ptr,id);
			return Mutex(&ptr->m_ftx,true);
		}

	}
	else {
		return Mutex(&ptr->m_ftx,false);
	}
}


} // end namespace IPC

} // namespace CppGenerics
