#include "../StdAfx.h"

#include "XcpPropertyBag.h"

#ifdef _MSC_VER
#pragma warning (disable : 4100)
#endif

XcpPropertyBag::XcpPropertyBag(){m_RefCount=0;}
XcpPropertyBag::~XcpPropertyBag(){}

HRESULT STDMETHODCALLTYPE XcpPropertyBag::QueryInterface(REFIID iid, void** ppvObject)
{
	return S_OK;
}

ULONG STDMETHODCALLTYPE XcpPropertyBag::AddRef(void)
{
	return InterlockedIncrement((LONG*)&m_RefCount);
}

ULONG STDMETHODCALLTYPE XcpPropertyBag::Release(void)
{
	int		newRefValue;
	
	newRefValue = InterlockedDecrement((LONG*)&m_RefCount);
	if (newRefValue == 0)
	{
		delete this;
		return 0;
	}
	
	return newRefValue;
}

STDMETHODIMP XcpPropertyBag::Read(LPCOLESTR pszPropName, VARIANT *pVar, IErrorLog *pErrorLog)
{
	HRESULT hr = E_INVALIDARG;
	BSTR bstrValue = NULL;
	  
	if (_wcsicmp(pszPropName, L"Source") == 0) 
	{
		bstrValue = SysAllocString(L"SilverlightBalls.xap");
	}    
	else if (_wcsicmp(pszPropName, L"Background") == 0) 
	{
		bstrValue = SysAllocString(L"Transparent");
	}
	else if (_wcsicmp(pszPropName, L"EnableGPUAcceleration") == 0) 
	{
		V_VT(pVar) = VT_BOOL;
		V_BOOL(pVar) = VARIANT_TRUE;
		hr = S_OK;
	}

	else if (_wcsicmp(pszPropName, L"Windowless") == 0) 
	{
		V_VT(pVar) = VT_BOOL;
		V_BOOL(pVar) = VARIANT_FALSE;
		hr = S_OK;
	}

	if (bstrValue != NULL) 
	{
		V_VT(pVar) = VT_BSTR;
		V_BSTR(pVar) = bstrValue;
		hr = S_OK;
	}
	return hr;
}

	
STDMETHODIMP XcpPropertyBag::Write(LPCOLESTR pszPropName, VARIANT *pVar)
{
	return S_OK;
}