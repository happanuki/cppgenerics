#pragma once

#include <exception>
#include <cinttypes>

#define IPC_MUTEXMANAGER_DESCRIPTOR_MAX_LEN 50
#define IPC_MUTEXMANAGER_CAPACITY 1000

namespace IPC {

typedef uint32_t __attribute__ ((aligned(4))) futex_id_t  ;


class Mutex
{
	futex_id_t* m_futexId = nullptr;

public:
	Mutex() {}
	Mutex(const Mutex&) = default;
	Mutex& operator=(const Mutex&) = default;
	Mutex(Mutex&&) = default;

	Mutex(futex_id_t* ptr, bool isNew = false);
	~Mutex() {}

	void lock() throw (std::exception&);
	void unlock() noexcept;
	inline futex_id_t* getPtr() noexcept { return m_futexId; }
};

} //namespace

