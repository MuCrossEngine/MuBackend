// CriticalSection.h: interface for the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class CCriticalSection
{
public:
	CCriticalSection();
	virtual ~CCriticalSection();
	void lock();
	void unlock();
private:
	std::mutex m_critical;
};
