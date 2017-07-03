#pragma once

namespace CppGenerics {


template < typename T> class ContainerIteratorPattern
{
public:
	using IteratorValueType = T;
	using Idx_t = unsigned long long;

	class Iterator;

	class Container
	{
	protected:
		friend class Iterator;

		virtual Iterator _getIterator(Idx_t index) = 0;
		virtual Idx_t _nextIdx(Idx_t index) = 0;
		virtual Idx_t _prevIdx(Idx_t index) = 0;

		virtual IteratorValueType& _getValue( Idx_t index ) = 0;

	public :
		virtual ~Container() = default;

		virtual Iterator begin() = 0;
		virtual Iterator end() = 0;
	};

	class Iterator
	{
		Container* m_cont = nullptr;
		Idx_t m_idx = 0 ;

	public:
		Iterator() = default;
		Iterator( Container* c, Idx_t idx ) : m_cont(c), m_idx(idx) {}

		virtual ~Iterator() {}

		virtual IteratorValueType& operator*() { return m_cont->_getValue( m_idx); }

		virtual const Iterator& operator++() { m_idx = m_cont->_nextIdx(m_idx); return *this; }
		virtual const Iterator& operator--() { m_idx = m_cont->_prevIdx(m_idx); return *this; }

		virtual bool operator==( const Iterator& rhs) { return m_cont == rhs.m_cont && m_idx == rhs.m_idx; }
		virtual bool operator!=( const Iterator& rhs) { return !( operator==(rhs)); }
	};

};



} // namespace CppGenerics
