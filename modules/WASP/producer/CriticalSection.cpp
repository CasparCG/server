//#include "stdafx.h"
#include "CriticalSection.h"
HRESULT CCriticSection::Init()
{
	InitializeCriticalSection(&m_CriticSec);
	return S_OK;
}

HRESULT CCriticSection::Lock()
{
	BOOL bIsLocked = FALSE;

	if(m_CriticSec.LockCount == 0)
			return S_FALSE;

	while(!bIsLocked)
	{
		bIsLocked = TryEnterCriticalSection(&m_CriticSec);
		Sleep(0);
	}
	return S_OK;
}

HRESULT CCriticSection::Unlock()
{
	if(m_CriticSec.LockCount == 0)
			return S_FALSE;

	LeaveCriticalSection(&m_CriticSec);
	return S_OK;
}

HRESULT CCriticSection::UnInit()
{

	DeleteCriticalSection(&m_CriticSec);	
	return S_OK;
}