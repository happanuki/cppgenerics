#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <linux/sched.h>
#include <stdlib.h>

#include <mutex>
#include <cstring>

#include "cppgenerics/IPC_Buffer.h"
#include "cppgenerics/FileLock.h"
#include "cppgenerics/Syscalls.h"
#include "cppgenerics/Exception.h"
#include "cppgenerics/Logger.h"

namespace IPC {


std::unique_ptr<BufferManager> instanceHolder (nullptr);

BufferManager& BufferManager::getInstance() throw (std::exception&)
{
	static std::mutex s_mtx;
	std::lock_guard<std::mutex> g(s_mtx);

	if (instanceHolder == nullptr) {

		(void) atexit( [](){ instanceHolder.reset(); } );

		instanceHolder.reset( new BufferManager() );
		if (instanceHolder == nullptr) {
			THROW_SYS_EXCEPTION("malloc failed");
		}

		instanceHolder->init();
	}

	return *instanceHolder;
}

BufferManager::BufferManager():
		m_capacity(IPC_BUFFER_MANAGER_CAPACITY),
		m_bytes( sizeof(*m_control) + m_capacity * sizeof(*m_mainStorage) )
{
}


void BufferManager::init() throw (std::exception&)
{
	auto fd = System::openAutoClose(IPC_BUFFER_MANAGER_FILE ,O_RDWR|O_CREAT,S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP);

	System::FileLock fdLock(*fd);
	std::lock_guard<System::FileLock> g(fdLock);

	int l_shm_key;

	auto bytes = System::read( *fd, &l_shm_key, sizeof(l_shm_key));
	if ( bytes == 0 ) {
		initPointers( 0 );
		initDescriptors();
		System::write(*fd, &m_control->shm_key, sizeof(m_control->shm_key));
	}
	else if ( bytes == sizeof(l_shm_key) )
	{
		initPointers( l_shm_key );
	}
	else {
		THROW_LOGIC_EXCEPTION("file contents junk");
	}
}


void BufferManager::initPointers(int shm_key) throw (std::exception&)
{
	int shmget_f = 0;

	if ( ! shm_key ) {
		//creating new Shared memory segment for memory descriptors;
		shm_key = allocShmKey() ;
		shmget_f = IPC_CREAT|IPC_EXCL| SHM_NORESERVE | S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP ;
	}


	m_shmId = shmget(shm_key, m_bytes, shmget_f);
	if ( m_shmId < 0) {
		THROW_SYS_EXCEPTION("shmget failed");
	}

	m_control = (BufferManager_Control*) shmat(m_shmId,nullptr,0);
	if ( m_control == (void*)-1) {
		THROW_SYS_EXCEPTION("shmat failed");
	}

	char* p1 = (char*) m_control + sizeof(*m_control);
	m_mainStorage = (BufferDescriptor*)p1;

	if ( shmget_f )
	{
		m_mtx = new Mutex(&m_control->futex,true);
		m_control->shm_key = shm_key;
		m_control->ref_count = 1;
		m_control->nextFreeIdx = 0;
		m_control->nextFullIdx =-1;
	}
	else
	{
		m_mtx = new Mutex(&m_control->futex,false);

		std::lock_guard<Mutex> g(*m_mtx);

		++(m_control->ref_count);
	}
}

void BufferManager::initDescriptors()
{
	for (auto i = 0; i < m_capacity; ++i ) {
		m_mainStorage[i].nextIdx = i + 1;
		m_mainStorage[i].prevIdx = i - 1;
	}
	m_mainStorage[ m_capacity - 1].nextIdx = -1;
}


int BufferManager::get_SysNumAttached() noexcept
{
	shmid_ds c_stats = { 0 };
	int ret = 0;
	if ( (ret = shmctl(m_shmId, IPC_STAT, &c_stats) ) < 0 ) {
		return ret;
	}
	else {
		return c_stats.shm_nattch;
	}
}


BufferManager::~BufferManager()
{
	m_mtx->lock();

	if ( !( --(m_control->ref_count)) || ( get_SysNumAttached() == 1) ) {
		//we're the last instance, lets explode it :)
		// first - destroy all error-left-descriptors (possible non-called destructors of allocated IPC::Buffers, std::terminate, f.e.)
		for (auto i = 0; i < m_capacity; ++i ) {
			BufferDescriptor& curDescriptor = m_mainStorage[i];
			if ( curDescriptor.id[0] ) {
				ERRSTDOUTT("EXPLODE undeleted descriptor : [ " + std::to_string(i) +" ]" +
						std::string(curDescriptor.id) );

				detachDescriptor(&curDescriptor,nullptr);
			}
		}

		(void) unlink(IPC_BUFFER_MANAGER_FILE);
		m_mtx->unlock();
		delete m_mtx;
		(void) shmdt(m_control);
		(void) shmctl(m_shmId, IPC_RMID, nullptr);
	}
	else {
		m_mtx->unlock();
		(void) shmdt(m_control);
		delete m_mtx;
	}
}


BufferManager::BufferDescriptor* BufferManager::getDescriptor(const std::string& id) noexcept
{
	DEBUGSTDOUTT("Searching ID : " + id);

	if ( m_control->nextFullIdx == -1) {
		return nullptr;
	}

	BufferDescriptor* fullPtr = &m_mainStorage[ m_control->nextFullIdx ];

	while ( fullPtr != nullptr )
	{
		if (! strncmp(id.c_str(), fullPtr->id, IPC_BUFFER_DESCRIPTOR_MAX_LEN )) {
			DEBUGSTDOUTT("Searched : " + id + "  found: " + std::string(fullPtr->id));

			return fullPtr;
		}

		if (fullPtr->nextIdx == -1 ) {
			fullPtr = nullptr;
		}
		else {
			fullPtr = &m_mainStorage[ fullPtr->nextIdx ];
		}
	}

	DEBUGSTDOUTT("Searched : " + id + " not found");
	return nullptr;
}


BufferManager::BufferDescriptor* BufferManager::getFreeDescriptor() noexcept
{
	if ( m_control->nextFreeIdx == -1) {
		return nullptr;
	}
	else {

		auto retIdx = m_control->nextFreeIdx;

		BufferDescriptor* ret = &m_mainStorage[ retIdx ];

		//Removing from prev list
		if ( ret->prevIdx != -1 ) {
			BufferDescriptor* prev = &m_mainStorage[ ret->prevIdx ];
			prev->nextIdx = ret->nextIdx ;
		}
		if ( ret->nextIdx != -1 ) {
			BufferDescriptor* next = &m_mainStorage[ ret->nextIdx ];
			next->prevIdx = ret->prevIdx ;
		}

		auto prevFullIdx = m_control->nextFullIdx;

		//fixing control
		m_control->nextFullIdx = retIdx ;
		m_control->nextFreeIdx = ret->nextIdx;

		//Switching ret
		if ( prevFullIdx != -1 ) {
			BufferDescriptor* prev = &m_mainStorage[ prevFullIdx ];
			prev->prevIdx = retIdx ;
			ret->prevIdx = -1;
		}
		ret->nextIdx = prevFullIdx;
		return ret;
	}
}


void BufferManager::putFreeDescriptor(BufferDescriptor* desc) noexcept
{
	int myIdx = ( (long) desc - (long)m_mainStorage ) / sizeof( *desc );

	//Removing from prev list
	if ( desc->prevIdx != -1 ) {
		BufferDescriptor* prev = &m_mainStorage[ desc->prevIdx ];
		prev->nextIdx = desc->nextIdx ;
	}
	if ( desc->nextIdx != -1 ) {
		BufferDescriptor* next = &m_mainStorage[ desc->nextIdx ];
		next->prevIdx = desc->prevIdx ;
	}

	memset(desc, 0, sizeof((*desc)));

	auto& prevFreeIdx = m_control->nextFreeIdx;

	if ( prevFreeIdx == -1) {
		prevFreeIdx = myIdx;
		desc->nextIdx = -1;
	}
	else {
		BufferDescriptor* prev = &m_mainStorage[ prevFreeIdx ];
		prev->prevIdx = myIdx;
		desc->nextIdx = prevFreeIdx;
	}
	desc->prevIdx = -1;
}


int BufferManager::allocShmKey()
{
	errno = 0;
	int key = 0;
	while ( errno != ENOENT) {
		key = System::getRandom(INT16_MAX,INT32_MAX);
		(void) shmget(key,1024,0);
	}
	return key;
}


void* BufferManager::attachDescriptor(BufferDescriptor* desc, bool isNew)
{
	int flags = ( isNew ) ? IPC_CREAT|IPC_EXCL|SHM_NORESERVE| S_IRUSR|S_IWUSR|S_IRGRP|S_IWGRP : 0 ;

	auto shmId = shmget(desc->shm_key, desc->bytes, flags);
	if ( shmId < 0) {
		THROW_SYS_EXCEPTION("shmget failed");
	}

	auto out = shmat(shmId,nullptr,0);
	if ( out == (void*)-1) {
		THROW_SYS_EXCEPTION("shmat failed");
	}

	m_localAtchs[desc] = out;

	return out;
}


void BufferManager::detachDescriptor(BufferDescriptor* bufDescriptor, void* mapAddr)
{
	auto shmId = shmget(bufDescriptor->shm_key, bufDescriptor->bytes,0);
	(void) shmdt(mapAddr);
	(void) shmctl(shmId, IPC_RMID, nullptr);

	putFreeDescriptor(bufDescriptor);

	m_localAtchs.erase(bufDescriptor);
}

/*
 * Buffer buf should be in locked state, unlocks here
 */
void BufferManager::bufferFree(Buffer* buf)
{
	DEBUGSTDOUTT(" freeing m_descriptor: " + std::to_string((long)buf->m_descriptor) + " refcount: " + std::to_string(buf->m_descriptor->ref_count));

	std::lock_guard<Mutex> g( *m_mtx);
	if (! (--buf->m_descriptor->ref_count) ) {

		DEBUGSTDOUTT(" detaching m_descriptor: " + std::to_string((long)buf->m_descriptor));
		delete buf->m_mtx;
		detachDescriptor(buf->m_descriptor, buf->m_ptr);
	}

	DEBUGSTDOUTT("END freeing m_descriptor: " + std::to_string((long)buf->m_descriptor) + " refcount: " + std::to_string(buf->m_descriptor->ref_count));

}


size_t BufferManager::RefCountInc_ts(BufferDescriptor* desc)
{
	std::lock_guard< IPC::Mutex> g(*m_mtx);

	++desc->ref_count;
	DEBUGSTDOUTT("refcount INC" + std::to_string(desc->ref_count));
	return desc->ref_count;
}


size_t BufferManager::RefCountDec_ts(BufferDescriptor* desc)
{
	std::lock_guard< IPC::Mutex> g(*m_mtx);
	--desc->ref_count;
	DEBUGSTDOUTT("refcount DEC" + std::to_string(desc->ref_count));
	return desc->ref_count;
}


size_t BufferManager::RefCountVal_ts(BufferDescriptor* desc)
{
	std::lock_guard< IPC::Mutex> g(*m_mtx);
	return desc->ref_count;
}



/*
 * Publics
 */


/*
 * - returns newly allocated or maps already existing buffer
 * - @bytes should be > 0 for new allocations
 * - mapped buffer size != @bytes , can be read from returned Buffer.getSize()
 */
Buffer BufferManager::getBuffer(const std::string& id, size_t bytes) throw (std::exception&)
{
	std::lock_guard<Mutex> g( *m_mtx);
	auto descriptorPtr = getDescriptor(id);
	if ( descriptorPtr == nullptr ) {
		//creating new buffer

		if(! bytes) {
			THROW_LOGIC_EXCEPTION("0-bytes size for new Descriptor");
		}

		auto freeDescriptorPtr = getFreeDescriptor();
		if ( freeDescriptorPtr == nullptr ) {
			//no free descriptors
			THROW_LOGIC_EXCEPTION("no free BufferDescriptor");
		}

		freeDescriptorPtr->bytes = bytes + sizeof(futex_id_t);
		freeDescriptorPtr->ref_count = 0;
		strncpy( freeDescriptorPtr->id, id.c_str(), IPC_BUFFER_DESCRIPTOR_MAX_LEN);
		freeDescriptorPtr->shm_key = allocShmKey();

		auto localPtr = attachDescriptor(freeDescriptorPtr, true);

		++freeDescriptorPtr->ref_count;

		DEBUGSTDOUT("INI Descriptor :" + std::to_string((long)freeDescriptorPtr) +
							"  Current refcount : " +std::to_string(freeDescriptorPtr->ref_count));

		return Buffer(localPtr,true,freeDescriptorPtr);
	}
	else {
		//map existent buffer
		//first - look in localy attached map
		void* localPtr = nullptr;

		auto found = m_localAtchs.find(descriptorPtr);
		if ( found == m_localAtchs.end()  ) {
			localPtr = attachDescriptor(descriptorPtr, false);
		}
		else
		{
			localPtr = found->second;
		}
		++descriptorPtr->ref_count;
		DEBUGSTDOUT("MAP Descriptor :" + std::to_string((long)descriptorPtr) +
					"  Current refcount : " +std::to_string(descriptorPtr->ref_count));

		return Buffer(localPtr,false,descriptorPtr);
	}
}


/*
 * Buffer impl
 */


Buffer::Buffer(void* ptr, bool isMtxNew, BufferManager::BufferDescriptor* desc ):
		m_size(desc->bytes - sizeof(futex_id_t) ),
		m_ptr( static_cast<futex_id_t*>(ptr) ),
		m_sptr( static_cast<char*>(ptr) + sizeof(futex_id_t) ),
		m_descriptor(desc),
		m_isCreator(isMtxNew)
{
	m_mtx = new Mutex(m_ptr,isMtxNew);
}

void Buffer::BufferInit(const Buffer& o)
{
	m_mtx = o.m_mtx;
	m_size = o.m_size;
	m_ptr = o.m_ptr;
	m_sptr = o.m_sptr;
	m_descriptor = o.m_descriptor;
	m_isCreator = o.m_isCreator;
}

Buffer::Buffer(Buffer&& o)
{
	auto& lvl = o;
	BufferInit(lvl);
	BufferManager::getInstance().RefCountInc_ts(m_descriptor);
}


Buffer::Buffer(const Buffer& o)
{
	BufferInit(o);
	BufferManager::getInstance().RefCountInc_ts(m_descriptor);
}


Buffer& Buffer::operator=(const Buffer& o)
{
	if ( m_ptr != nullptr ) {
		//first - "delete" self
		BufferManager::getInstance().bufferFree(this);
	}

	BufferInit(o);
	BufferManager::getInstance().RefCountInc_ts(m_descriptor);
	return *this;
}


Buffer::~Buffer()
{
	BufferManager::getInstance().bufferFree(this);
}








} //namespace
