// CriticalSection.cpp: implementation of the CCriticalSection class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "CriticalSection.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CCriticalSection::CCriticalSection() // OK
{
}

CCriticalSection::~CCriticalSection() // OK
{
}

void CCriticalSection::lock() // OK
{
	this->m_critical.lock();
}

void CCriticalSection::unlock() // OK
{
	this->m_critical.unlock();
}
