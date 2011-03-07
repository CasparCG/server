#include "../StdAfx.h"

#include "XcpControlHost.h"
#include "XcpPropertyBag.h"
#include "Ocidl.h"
#include "xcpctrl_h.h"

#include "atlstr.h"

using namespace ATL;

#ifdef _MSC_VER
#pragma warning (disable : 4100)
#pragma warning(disable:4061)
#endif

XcpControlHost::XcpControlHost() 
	: hControlWindow(nullptr)
	, m_pUnKnown(nullptr)
	, m_pControl(nullptr)
{
}

XcpControlHost::~XcpControlHost() 
{
}

STDMETHODIMP XcpControlHost::GetHostOptions(DWORD* pdwOptions)
{
	*pdwOptions = XcpHostOption_EnableCrossDomainDownloads|
		XcpHostOption_EnableScriptableObjectAccess|XcpHostOption_EnableHtmlDomAccess;
	return S_OK;
}

STDMETHODIMP XcpControlHost::GetCustomAppDomain(IUnknown** ppAppDomain) 
{  
	return S_OK;
}

STDMETHODIMP XcpControlHost::GetControlVersion(UINT *puMajorVersion, UINT *puMinorVersion)
{
	*puMajorVersion = 4;
	*puMinorVersion = 50401;
	return S_OK;;
}

STDMETHODIMP XcpControlHost::NotifyLoaded() 
{
//	MessageBox(L"Notify Loaded", L"Silverlight Error", 0);
	return S_OK;
}

STDMETHODIMP XcpControlHost::NotifyError(BSTR bstrError, BSTR bstrSource, long nLine, long nColumn)
{
	return S_OK;
}

STDMETHODIMP XcpControlHost::InvokeHandler(BSTR bstrName, VARIANT varParam1, VARIANT varParam2, VARIANT* pvarResult) 
{
	return E_NOTIMPL;
}

STDMETHODIMP XcpControlHost::GetBaseUrl(BSTR* pbstrUrl) {

	CAtlString strPath = "C:\\";
	//TCHAR pBuff[255];
	//GetCurrentDirectory(255, pBuff);
	//strPath = pBuff;
	//strPath += "\\";

	*pbstrUrl = SysAllocString(strPath);

	//MessageBox(*pbstrUrl, L"GetBaseUrl", 0);
	return S_OK;
}

STDMETHODIMP XcpControlHost::GetNamedSource(BSTR bstrSourceName, BSTR* pbstrSource) 
{    
	return E_NOTIMPL;
}

STDMETHODIMP XcpControlHost::DownloadUrl(BSTR bstrUrl, IXcpControlDownloadCallback* pCallback, IStream** ppStream) 
{
//	MessageBox(L"DownloadUrl", L"DownloadUrl", 0);
	return S_FALSE;
}

///////////////////////////////////////////////////////////////////////////////
// XcpControlHost IServiceProvider Implementation

STDMETHODIMP XcpControlHost::QueryService(REFGUID rsid, REFIID riid, void** ppvObj) {
	ATLASSERT(ppvObj != NULL);
	if (ppvObj == NULL)
		return E_POINTER;
	*ppvObj = NULL;

	//static const GUID IID_IXcpControlHost = 
	//	{ 0x1B36028E, 0xB491, 0x4bb2, { 0x85, 0x84, 0x8A, 0x9E, 0x0A, 0x67, 0x7D, 0x6E }};

	HRESULT hr = E_NOINTERFACE;

	if ((rsid == IID_IXcpControlHost) && (riid == IID_IXcpControlHost)) {
		((IXcpControlHost*)this)->AddRef();
		*ppvObj = (IXcpControlHost*)this;
		hr = S_OK;
	}

	if ((rsid == IID_IXcpControlHost2) && (riid == IID_IXcpControlHost2)) {
		((IXcpControlHost2*)this)->AddRef();
		*ppvObj = (IXcpControlHost2*)this;
		hr = S_OK;
	}


	return hr;
}

///////////////////////////////////////////////////////////////////////////////
// General ActiveX control embedding.


HRESULT XcpControlHost::CreateXcpControl(HWND hWnd) 
{
	AtlAxWinInit();

	HRESULT hr;
	static const GUID IID_IXcpControl = 
		{ 0x1FB839CC, 0x116C, 0x4C9B, { 0xAE, 0x8E, 0x3D, 0xBB, 0x64, 0x96, 0xE3, 0x26 }};

	static const GUID CLSID_XcpControl = 
		{ 0xDFEAF541, 0xF3E1, 0x4c24, { 0xAC, 0xAC, 0x99, 0xC3, 0x07, 0x15, 0x08, 0x4A }};

	static const GUID IID_IXcpControl2 = 
		{ 0x1c3294f9, 0x891f, 0x49b1, { 0xBB, 0xAE, 0x49, 0x2a, 0x68, 0xBA, 0x10, 0xcc }};

//	static const GUID CLSID_XcpControl2 = 
//		{ 0xDFEAF541, 0xF3E1, 0x4c24, { 0xAC, 0xAC, 0x99, 0xC3, 0x07, 0x15, 0x08, 0x4A }};

	//static const GUID IID_IXcpControlHost2 = 
	//	{ 0xfb3ed7c4, 0x5797, 0x4b44, { 0x86, 0x95, 0x0c, 0x51, 0x2e, 0xa2, 0x7D, 0x8f }};
	
	hr = CoCreateInstance(CLSID_XcpControl, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_pUnKnown);
	
	if (SUCCEEDED(hr)) 
	{
		CComPtr<IUnknown> spUnkContainer;
		hr = XcpControlHost::_CreatorClass::CreateInstance(NULL, IID_IUnknown, (void**)&spUnkContainer);
		if (SUCCEEDED(hr)) 
		{
			CComPtr<IAxWinHostWindow> pAxWindow;

			spUnkContainer->QueryInterface(IID_IAxWinHostWindow, (void**)&pAxWindow);
			m_pUnKnown->QueryInterface(IID_IXcpControl2, (void**)&m_pControl);
			hr = pAxWindow->AttachControl(m_pUnKnown, hWnd);            
			hControlWindow = hWnd;

			IOleInPlaceActiveObject *pObj;
			hr = m_pControl->QueryInterface(IID_IOleInPlaceActiveObject, (void**)&pObj);
		}
	}
	return hr;
}

HRESULT XcpControlHost::DestroyXcpControl()
{
	HRESULT hr = S_OK;
	if (m_pControl)
	{
		m_pControl->Release();
	}
	if (m_pUnKnown)
	{
		m_pUnKnown->Release();
	}
	return hr;
}


STDMETHODIMP XcpControlHost::AttachControl(IUnknown* pUnKnown, HWND hWnd) {
	ReleaseAll();

	HRESULT hr = S_FALSE;
	BOOL fReleaseWindowOnFailure = FALSE;

	if ((m_hWnd != NULL) && (m_hWnd != hWnd)) {
		// Don't release the window if it's the same as the one we already subclass/own
		RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
		ReleaseWindow();
	}

	if (::IsWindow(hWnd)) {
		if (m_hWnd != hWnd) {
			// Don't need to subclass the window if we already own it
			SubclassWindow(hWnd);
			fReleaseWindowOnFailure = TRUE;
		}

		hr = ActivateXcpControl(pUnKnown);
		if (FAILED(hr)) {
			ReleaseAll();

			if (m_hWnd != NULL) {
				RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);

				if (fReleaseWindowOnFailure) {
					// We subclassed the window in an attempt to create this control, so we unsubclass on failure
					ReleaseWindow();
				}
			}
		}
	}
	return hr;
}

HRESULT XcpControlHost::ActivateXcpControl(IUnknown* pUnKnown) 
{
	if (pUnKnown == NULL)
	{
		return S_OK;
	}

	m_spUnknown = pUnKnown;

	HRESULT hr = S_OK;
	pUnKnown->QueryInterface(__uuidof(IOleObject), (void**)&m_spOleObject);
	if (m_spOleObject) 
	{
		m_spOleObject->GetMiscStatus(DVASPECT_CONTENT, &m_dwMiscStatus);
		if (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST) 
		{
			CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
			m_spOleObject->SetClientSite(spClientSite);
		}

		CComQIPtr<IPersistPropertyBag> pPersist(m_spOleObject);

		if (pPersist != NULL) 
		{
			IPropertyBag* pPropBag = new XcpPropertyBag();
			pPropBag->AddRef();

			pPersist->Load((IPropertyBag*)pPropBag, NULL);
			pPropBag->Release();
		}

		if (0 == (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)) 
		{
			CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
			m_spOleObject->SetClientSite(spClientSite);
		}

		m_dwViewObjectType = 0;
		hr = m_spOleObject->QueryInterface(__uuidof(IViewObjectEx), (void**) &m_spViewObject);
		if (FAILED(hr)) 
		{
			hr = m_spOleObject->QueryInterface(__uuidof(IViewObject2), (void**) &m_spViewObject);
			if (SUCCEEDED(hr)) {
				m_dwViewObjectType = 3;
			}
		}
		else {
			m_dwViewObjectType = 7;
		}

		if (FAILED(hr)) 
		{
			hr = m_spOleObject->QueryInterface(__uuidof(IViewObject), (void**) &m_spViewObject);
			if (SUCCEEDED(hr))
				m_dwViewObjectType = 1;
		}

		CComQIPtr<IAdviseSink> spAdviseSink(GetControllingUnknown());
		m_spOleObject->Advise(spAdviseSink, &m_dwOleObject);
		if (m_spViewObject) 
		{
			m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, spAdviseSink);
		}

		m_spOleObject->SetHostNames(OLESTR("AXWIN"), NULL);

		if ((m_dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME) == 0) 
		{
			GetClientRect(&m_rcPos);

			m_pxSize.cx =  m_rcPos.right - m_rcPos.left;
			m_pxSize.cy =  m_rcPos.bottom - m_rcPos.top;
			AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
			m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
			m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
			AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
			m_rcPos.right = m_rcPos.left + m_pxSize.cx ;
			m_rcPos.bottom = m_rcPos.top + m_pxSize.cy ;

			CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
			hr = m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, spClientSite, 0, m_hWnd, &m_rcPos);
			RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW | RDW_ERASE | RDW_INTERNALPAINT | RDW_FRAME);
		}
	}

	CComPtr<IObjectWithSite> spSite;
	pUnKnown->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
	if (spSite != NULL) 
	{
		spSite->SetSite(GetControllingUnknown());
	}

	return hr;
}
