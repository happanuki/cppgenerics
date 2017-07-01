#pragma once

#include <exception>
#include <cinttypes>
#include <memory>
#include <map>

#include "cppgenerics/IPC_Mutex.h"

#define IPC_BUFFER_DESCRIPTOR_MAX_LEN 50
#define IPC_BUFFER_MANAGER_FILE "/tmp/.ipc.bufferManager.key"
#define IPC_BUFFER_MANAGER_CAPACITY 1000

namespace IPC
{

class Buffer;

class BufferManager
{
	friend class Buffer;

	using list_idx_t = int; // internal simple-and-fast-I-hope :) list

	struct BufferDescriptor
	{
		int shm_key = 0;
		size_t bytes = 0;
		size_t ref_count = 0;

		list_idx_t nextIdx; // internal list index, it can be next in free/full descriptors lists, but not in both, -1 for list's end
		list_idx_t prevIdx;

		char id[IPC_BUFFER_DESCRIPTOR_MAX_LEN + 1] = { 0 }; // should be the last to prevent incorrect aligning
	};

	struct BufferManager_Control
	{
		futex_id_t futex;
		int shm_key;
		size_t ref_count;

		list_idx_t nextFreeIdx = 0; // internal list of free descriptors
		list_idx_t nextFullIdx = -1; // internal list of full descriptors
	};



	size_t m_capacity; // total number of embedded Units (e.g. BufferDescriptor's)
	size_t m_bytes; // total size in bytes :)

	BufferManager_Control* m_control = nullptr; // start of VMA
	BufferDescriptor* m_mainStorage = nullptr; // VMA + BufferManager_Control, array of BufferDescriptor's

	Mutex* m_mtx = nullptr;

	int m_shmId = -1;

	std::map<BufferDescriptor*, void*> m_localAtchs;

	int allocShmKey();

	/*
	 * internal list initial chaining
	 */
	void initDescriptors();
	void initPointers(int shm_key) throw (std::exception&);
	void init() throw (std::exception&);

	BufferDescriptor* getDescriptor(const std::string& id) noexcept;
	BufferDescriptor* getFreeDescriptor() noexcept;
	void putFreeDescriptor(BufferDescriptor* desc) noexcept;

	void* attachDescriptor(BufferDescriptor* desc, bool isNew = false);
	void  detachDescriptor(BufferDescriptor* desc, void* mapAddr);

	void bufferFree(Buffer* ptr);

	int get_SysNumAttached() noexcept;

	size_t RefCountInc_ts(BufferDescriptor* desc);
	size_t RefCountDec_ts(BufferDescriptor* desc);
	size_t RefCountVal_ts(BufferDescriptor* desc);

	BufferManager();

public:
	static BufferManager& getInstance() throw (std::exception&);

	BufferManager(const BufferManager&) = delete;
	BufferManager(BufferManager&&) = delete;
	BufferManager& operator=(const BufferManager&) = delete;
	~BufferManager();

	/*
	 * - returns newly allocated or maps already existing buffer
	 * - @bytes should be > 0 for new allocations
	 * - mapped buffer size != @bytes , can be read from returned Buffer.getSize()
	 */
	Buffer getBuffer(const std::string& id, size_t bytes) throw (std::exception&);
};


class Buffer
{
	friend class BufferManager;

	Mutex* m_mtx = nullptr; //IPC mutex
	size_t m_size = 0; //size
	futex_id_t* m_ptr = nullptr; //local pointer to mapped SHM, futex storage
	void* m_sptr = nullptr; //storage pointer, m_ptr + sizeof(futex_id_t)
	BufferManager::BufferDescriptor* m_descriptor = nullptr;
	bool m_isCreator = false;

	void BufferInit(const Buffer& o);
	Buffer(void* ptr, bool isMtxNew, BufferManager::BufferDescriptor* desc );

public:
	Buffer() {};

	Buffer(Buffer&&);
	Buffer& operator=(const Buffer&);
	Buffer(const Buffer&);

	~Buffer();

	inline void lock() throw (std::exception&) { m_mtx->lock(); }
	inline void unlock() noexcept { m_mtx->unlock(); }

	inline size_t getSize() noexcept { return m_size; }
	inline void* getPtr() noexcept { return m_sptr; }
	inline bool isCreator() noexcept { return m_isCreator; }
};


} //namespace
