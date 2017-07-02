#pragma once

namespace CppGenerics {

template <typename T> class ObserverPattern
{
public:
	class Observer_I
	{
		public:
			virtual ~Observer_I() {}

			virtual void notifyHandler(const T&) = 0;
	};

	class Subject_I
	{
		public:
			virtual ~Subject_I() {}

			virtual void addObs(Observer_I&) = 0;
			virtual void rmObs(Observer_I&) = 0;
			virtual void notifyObs(const T&) = 0;
	};

	ObserverPattern() = delete;
};

} // namespace CppGenerics
