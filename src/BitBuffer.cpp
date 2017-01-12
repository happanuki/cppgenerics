#include <unistd.h>

#include <cstdlib>
#include <string>

#include "BitBuffer.h"
#include "Exception.h"
#include "Logger.h"

size_t BitBuffer::getSizeBytes(size_t numElems) noexcept
{
	auto elemsPerUnit = sizeof(storeUnit_t) * 8;

	auto out = numElems / elemsPerUnit ;

	if ( numElems % elemsPerUnit ) {
		++out;
	}

	return out * sizeof(storeUnit_t);
}


BitBuffer::BitBuffer(size_t numElements, void* storage):
		m_numElements(numElements),
		m_storage( static_cast<storeUnit_t*>(storage) )
{
	m_numUnits = m_numElements / c_unitBits ;
	m_lastUnitBits =  m_numElements % c_unitBits ;

	if ( m_lastUnitBits ) {
		++m_numUnits;
	}
}


void BitBuffer::freeMyMem() noexcept
{
	if (m_isSelfAllocatedStorage) {
		delete[] m_storage;
	}
}


BitBuffer::~BitBuffer()
{
	freeMyMem();
}


void BitBuffer::init() throw (std::exception&)
{
	if ( m_storage == nullptr) {
		m_isSelfAllocatedStorage = true;
		m_storage = (storeUnit_t*) malloc(m_numUnits * c_unitBytes);
		if ( m_storage == nullptr) {
			THROW_SYS_EXCEPTION("calloc");
		}
	}

	memset(m_storage, 0xFF, m_numUnits * c_unitBytes);

	if (m_lastUnitBits) {
		storeUnit_t& lastUnit = m_storage[m_numUnits - 1];
		lastUnit = FULL_BIT ;
		// adjust last unit
		for ( size_t i = 0; i < m_lastUnitBits ; ++i ) {
			lastUnit |= FREE_BIT << i ;
		}
	}

}


void BitBuffer::checkIdx(size_t idx) throw (std::exception&)
{
	if (idx > (m_numElements-1) ) {
		THROW_LOGIC_EXCEPTION(" BitBuffer::checkIdx() failed, idx > m_numElements-1  : " + std::to_string(idx) + " > " + std::to_string(m_numElements - 1));
	}
}

void BitBuffer::checkNumber(size_t num) throw (std::exception&)
{
	if (num > m_numElements ) {
		THROW_LOGIC_EXCEPTION(" BitBuffer::checkIdx() failed, num > m_numElements  : " + std::to_string(num) + " > " + std::to_string(m_numElements));
	}
}


bool BitBuffer::get(size_t idx) throw (std::exception&)
{
	checkIdx(idx);

	auto unitIdx = idx / c_unitBits ;
	auto bitIdx = idx % c_unitBits ;

	return m_storage[unitIdx] & ( 1 << bitIdx ) ;
}


void BitBuffer::set(size_t idx, bool val) throw (std::exception&)
{
	checkIdx(idx);

	auto unitIdx = idx / c_unitBits ;
	auto bitIdx = idx % c_unitBits ;

	if ( val ) {
		m_storage[unitIdx] |= (1 << bitIdx) ;
	}
	else {
		m_storage[unitIdx] &= ~(1 << bitIdx) ;
	}
}


/*
 * 	returns idx of continuous free region num- elements long
 * 	-1 if not found
 */
long BitBuffer::find(size_t num) throw (std::exception&)
{
	checkNumber(num);

	decltype(num) foundNum = 0;
	long foundUnitIdx = -1;
	long curIdx = 0;

	while ( ( foundNum < num ) && ( curIdx < m_numElements ) ) {

		if ( get(curIdx) ) {
			++foundNum;
			foundUnitIdx = curIdx;
		}
		else {
			foundNum = 0;
		}

		++curIdx;
	}

	auto out = -1;

	if ( foundNum == num ) { //found region
		out = (foundUnitIdx + 1) - num ;
	}

	return out;
}


void BitBuffer::lock() throw (std::exception&)
{
}


void BitBuffer::unlock() noexcept
{
}


void BitBuffer::createOther(const BitBuffer& o) throw (std::exception&)
{
	c_unitBytes = o.c_unitBytes;
	c_unitBits = o.c_unitBits;

	m_numElements = o.m_numElements;
	m_numUnits = o.m_numUnits;
	m_lastUnitBits = o.m_lastUnitBits;

	m_isSelfAllocatedStorage = o.m_isSelfAllocatedStorage;

	if (o.m_isSelfAllocatedStorage) {
		//copy storage
		auto sz = m_numUnits * c_unitBytes;
		m_storage = (storeUnit_t*) malloc(sz);
		memcpy(m_storage, o.m_storage,sz);
	}
	else {
		m_storage = o.m_storage;
	}
}


BitBuffer::BitBuffer(const BitBuffer& o)
{
	createOther(o);
}


BitBuffer::BitBuffer(BitBuffer&& o)
{
	auto old = o.m_isSelfAllocatedStorage;
	o.m_isSelfAllocatedStorage = false; // to prevent deletion in o

	createOther(o);

	m_isSelfAllocatedStorage = old;
}


BitBuffer& BitBuffer::operator=(const BitBuffer& o)
{
	freeMyMem();
	createOther(o);
	return *this;
}
