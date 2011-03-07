#pragma once

#include "Ocidl.h"
#include "xcpctrl_h.h"

#include "atlstr.h"

using namespace ATL;

class XcpPropertyBag : public IPropertyBag
{
	int m_RefCount;
public:
	XcpPropertyBag();
	~XcpPropertyBag();

	HRESULT _stdcall QueryInterface(REFIID iid, void** ppvObject);
	ULONG _stdcall AddRef();	
	ULONG _stdcall Release();

	STDMETHOD (Read)(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog);	
	STDMETHOD (Write)(LPCOLESTR pszPropName, VARIANT *pVar);
};