#pragma once

#include <cstddef>
#include <exception>
#include <string>
#include <cstring>
#include <mutex>

#include "cppgenerics/BitBuffer.h"
#include "cppgenerics/IPC_Buffer.h"
#include "cppgenerics/Logger.h"

namespace CppGenerics {

namespace IPC {

template <class T> class Allocator
{
	std::string m_storageKey;
	std::string m_modelKey;

	Buffer m_storageBuffer;
	Buffer m_modelBuffer;

	BitBuffer m_model;

public:
	typedef T value_type;

	Allocator(const std::string& storageKey, const std::string& modelKey, size_t maxBytes = 0) throw (std::exception&);
	template <class U> Allocator(const Allocator<U>& o);

	T* allocate(std::size_t numElems);
	void deallocate(T* p, std::size_t numElems);
	/////////

	inline size_t getSize() noexcept { return m_storageBuffer.getSize(); }
	inline size_t getNumElements() noexcept { return getSize() / sizeof(value_type); }
	inline void* getStoragePtr() noexcept { return m_storageBuffer.getPtr(); }
};

namespace
{
	template <class T, class U> bool operator==(const IPC::Allocator<T>&, const IPC::Allocator<U>&);
	template <class T, class U> bool operator!=(const IPC::Allocator<T>&, const IPC::Allocator<U>&);
}


////////

template <class T> Allocator<T>::Allocator(const std::string& storageKey, const std::string& modelKey, size_t maxBytes) throw (std::exception&) :
		m_storageKey(storageKey),
		m_modelKey(modelKey)
{

	m_storageBuffer = IPC::BufferManager::getInstance().getBuffer(m_storageKey,maxBytes);

	size_t modelBytes = BitBuffer::getSizeBytes( getNumElements());
	DEBUGSTDOUTT(" modelBytes = " << modelBytes );

	m_modelBuffer = IPC::BufferManager::getInstance().getBuffer(m_modelKey,modelBytes);

	m_model = BitBuffer( getNumElements(), m_modelBuffer.getPtr() );
	m_model.init();
}


template <class T> template <class U> Allocator<T>::Allocator(const Allocator<U>& o)
{
	m_storageKey = o.m_storageKey;
	m_modelKey = o.m_modelKey;

	m_storageBuffer = o.m_storageBuffer;
	m_modelBuffer = o.m_modelBuffer;

	m_model = o.m_model;
}


template <class T> T* Allocator<T>::allocate(std::size_t numElems)
{
	std::lock_guard< Buffer > g(m_modelBuffer);

	DEBUGSTDOUTT("Allocating " << numElems << " elements");

	auto idx = m_model.find(numElems);
	if ( idx < 0) {
		ERRSTDOUTT("model search to place "  << numElems << " elements failed");
		return nullptr;
	}

	for (int i=0; i<numElems; ++i) {
		m_model.set(idx+i,false);
	}

	auto bytesOffset = idx * sizeof(T);
	T* outPtr = reinterpret_cast<T*>( (char*) m_storageBuffer.getPtr() + bytesOffset);

	return outPtr;
}


template <class T> void Allocator<T>::deallocate(T* p, std::size_t numElems)
{
	std::lock_guard< Buffer > g(m_modelBuffer);
	std::lock_guard< Buffer > gg(m_storageBuffer);

	auto idx = ( (long)p - (long)m_storageBuffer.getPtr() ) / sizeof(T) ;

	for (int i = idx; i < idx+numElems; ++i) {
		m_model.set(idx+i,true);
	}

	(void) memset(p, 0, numElems * sizeof(T) );
}


namespace
{

template <class T, class U> bool operator==(const IPC::Allocator<T>& lhs, const IPC::Allocator<U>& rhs)
{
	return 	( lhs.m_modelKey == rhs.m_modelKey) && \
			( lhs.m_storageKey == rhs.m_storageKey) && \
			( sizeof(T) == sizeof(U) );
}


template <class T, class U> bool operator!=(const IPC::Allocator<T>& lhs, const IPC::Allocator<U>& rhs)
{
	return ! operator==(lhs,rhs);
}

} //namespace -


} //namespace IPC


} // namespace CppGenerics
