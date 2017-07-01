#pragma once
#include <iterator>
#include <mutex>
#include <string>

#include "cppgenerics/IPC_Buffer.h"

namespace IPC {

/*
 * Thread safe container and iterator
 */

template <class T> class Container
{
	Buffer m_buffer;

public:
	using m_iterator_t = std::iterator< std::input_iterator_tag, T,long, T*, T& > ;

	class Iterator : public m_iterator_t
	{
		typename m_iterator_t::difference_type m_pos = 0;

		T* m_buf = nullptr;
		Buffer* m_storage = nullptr;

		typename m_iterator_t::difference_type m_getMaxElems() { return m_storage->getSize() / sizeof(T) ; }

	public:
		Iterator(Buffer* storage, typename m_iterator_t::difference_type pos = 0)
			: m_pos(pos), m_storage(storage)
		{
			m_buf = reinterpret_cast<typename m_iterator_t::pointer>( m_storage->getPtr());
		}
		Iterator() = default;
		Iterator(const Iterator&) = default;
		Iterator& operator=(const Iterator&) = default;
		~Iterator() {}
		Iterator(Iterator&&) = default;

        Iterator& operator++() { ++m_pos; return *this; }
        Iterator operator++(int) { Iterator retval = *this; ++(*this); return retval;}
        bool operator==(const Iterator& other) const {return ( m_buf == other.m_buf ) && (m_pos == other.m_pos); }
        bool operator!=(const Iterator& other) const {return !(*this == other); }
        typename m_iterator_t::reference operator*() const { std::lock_guard<Buffer> g(*m_storage); return m_buf[m_pos]; }

		//http://en.cppreference.com/w/cpp/concept/RandomAccessIterator
		Iterator& operator+=(typename m_iterator_t::difference_type m) {
			auto& r = *this;
			if (m >= 0) while (m--) ++r;
			else while (m++) --r;
			return r;
		}

//		Iterator operator+(Iterator::difference_type )

	};


	//Container

	Container(const Buffer& b) : m_buffer(b) {}
	Container(const std::string& bufName, typename Iterator::difference_type maxElems )
	{
		m_buffer = BufferManager::getInstance().getBuffer(bufName, maxElems * sizeof(T) );

	}

	Iterator begin() { return Iterator(&m_buffer,0); }
	Iterator end() { return Iterator(&m_buffer,m_buffer.getSize() / sizeof(T)); }

	T& operator[](typename Iterator::difference_type idx) {
		auto ptr = reinterpret_cast<T*>( m_buffer.getPtr() );
		return ptr[ idx*sizeof(T) ];
	}

	Buffer& getBuffer() { return m_buffer; }

};
} //namespace
