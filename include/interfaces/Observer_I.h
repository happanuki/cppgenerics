#pragma once

class Subject_I;

class SubjResult_I
{
public:
	virtual ~SubjResult_I() {}
};

class Observer_I
{
public:
	virtual ~Observer_I() {}

	virtual void notifyHandler(const SubjResult_I&) = 0;
};

class Subject_I
{
public:
	virtual ~Subject_I() {}

	virtual void addObs(Observer_I&) = 0;
	virtual void rmObs(Observer_I&) = 0;
	virtual void notifyObs(const SubjResult_I&) = 0;
};
