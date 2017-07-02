#pragma once

#include <memory>
#include <list>

namespace CppGenerics {


template <typename T> class ObserverPattern
{
public:
	ObserverPattern() = delete;

	class Observer_I
	{
	public:
			virtual ~Observer_I() {}
			virtual void notifyHandler(const T&) = 0;
	};

	class Subject_I
	{
	public:
		std::list<Observer_I*> m_obses;

		virtual ~Subject_I() {}
		/*
		 * IT IS RESPONSIBLE of IMPLEMENTATION to maintain Observer_I* valid
		 * between addObs() and rmObs() calls
		 */
		virtual void addObs(Observer_I* o) { m_obses.push_back(o); }

		virtual void rmObs(Observer_I* o) { m_obses.remove(o); }

		virtual void notifyObses(const T& val) {
			for ( auto& it: m_obses )
				it->notifyHandler(val);
		}
	};
};

} // namespace CppGenerics
