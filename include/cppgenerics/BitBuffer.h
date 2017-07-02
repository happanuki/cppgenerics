#pragma once

#include <cstddef>
#include <exception>


namespace CppGenerics {

class BitBuffer
{
	using storeUnit_t = ulong;
	uint c_unitBytes = sizeof(storeUnit_t);
	uint c_unitBits = c_unitBytes * 8;

	storeUnit_t* m_storage = nullptr;
	bool m_isSelfAllocatedStorage = false;

	size_t m_numElements;
	size_t m_numUnits = 0;

	size_t m_lastUnitBits = 0;

	void checkIdx(size_t idx) throw (std::exception&);
	void checkNumber(size_t idx) throw (std::exception&);

	void createOther(const BitBuffer& o) throw (std::exception&);
	void freeMyMem() noexcept;

public:
	static const int FREE_BIT; // 1
	static const int FULL_BIT; // 0

	static size_t getSizeBytes(size_t numElems) noexcept;

	BitBuffer(const BitBuffer&);
	BitBuffer(BitBuffer&&);
	BitBuffer& operator=(const BitBuffer&);

	BitBuffer(size_t numElements = 1, void* storage = nullptr);
	~BitBuffer();

	void init() throw (std::exception&);

	bool get(size_t idx) throw (std::exception&);
	void set(size_t idx, bool val) throw (std::exception&);

	void lock() throw (std::exception&);
	void unlock() noexcept;

	/*
	 * 	returns idx of continuous free region num- elements long
	 * 	-1 if not found
	 */
	long find(size_t num) throw (std::exception&);

	uint8_t* const getInternalStorage() const;
};

} //namespace CppGenerics
