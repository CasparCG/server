// XcpControlHost.cpp : Implementation of XcpControlHost

#include "XcpControlHost.h"
#include "XcpPropertyBag.h"
#include "Ocidl.h"
#include "xcpctrl_h.h"

#include "atlstr.h"

using namespace ATL;

#pragma warning(disable:4061)
#pragma warning(disable:4100)

XcpControlHost::XcpControlHost()	
	: m_UIActive(false)
	, m_bInPlaceActive(false)
	, m_bHaveFocus(false)
	, m_bCapture(false)
{
}

XcpControlHost::~XcpControlHost() 
{
}

///////////////////////////////////////////////////////////////////////////////
// IXcpControlHost2 
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP XcpControlHost::GetHostOptions(DWORD* pdwOptions)
{
	*pdwOptions = XcpHostOption_EnableCrossDomainDownloads | XcpHostOption_EnableScriptableObjectAccess | XcpHostOption_EnableHtmlDomAccess;
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

STDMETHODIMP XcpControlHost::GetBaseUrl(BSTR* pbstrUrl)
{	
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

///////////////////
// IObjectWithSite
///////////////////
HRESULT STDMETHODCALLTYPE XcpControlHost::SetSite(IUnknown* pUnkSite)
{
	ATLTRACE(_T("IObjectWithSite::SetSite\n"));
	HRESULT hr = IObjectWithSiteImpl<XcpControlHost>::SetSite(pUnkSite);

	if (SUCCEEDED(hr) && m_spUnkSite)
	{
		// Look for "outer" IServiceProvider
		hr = m_spUnkSite->QueryInterface(__uuidof(IServiceProvider), (void**)&m_spServices);
		ATLASSERT( !hr && _T("No ServiceProvider!") );
	}

	if (pUnkSite == NULL)
		m_spServices.Release();

	return hr;
}

///////////////////
// IOleClientSite
///////////////////
HRESULT STDMETHODCALLTYPE XcpControlHost::SaveObject()
{
	ATLTRACENOTIMPL(_T("IOleClientSite::SaveObject"));
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk)
{
	ATLTRACENOTIMPL(_T("IOleClientSite::GetMoniker"));
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetContainer(IOleContainer** ppContainer)
{
	ATLTRACE(atlTraceHosting, 2, _T("IOleClientSite::GetContainer\n"));
	ATLASSERT(ppContainer != NULL);

	HRESULT hr = E_POINTER;
	if (ppContainer)
	{
		hr = E_NOTIMPL;
		(*ppContainer) = NULL;
		if (m_spUnkSite)
			hr = m_spUnkSite->QueryInterface(__uuidof(IOleContainer), (void**)ppContainer);
		if (FAILED(hr))
			hr = QueryInterface(__uuidof(IOleContainer), (void**)ppContainer);
	}
	return hr;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::ShowObject()
{
	//ATLTRACE(atlTraceHosting, 2, _T("IOleClientSite::ShowObject\r\n"));

	//HDC hdc = CWindowImpl<CAxHostWindow>::GetDC();
	//if (hdc == NULL)
	//	return E_FAIL;
	//if (m_spViewObject)
	//	m_spViewObject->Draw(DVASPECT_CONTENT, -1, NULL, NULL, NULL, hdc, (RECTL*)&m_rcPos, (RECTL*)&m_rcPos, NULL, 0);
	//CWindowImpl<CAxHostWindow>::ReleaseDC(hdc);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnShowWindow(BOOL fShow)
{
	ATLTRACENOTIMPL(_T("IOleClientSite::OnShowWindow"));
}

HRESULT STDMETHODCALLTYPE XcpControlHost::RequestNewObjectLayout()
{
	ATLTRACENOTIMPL(_T("IOleClientSite::RequestNewObjectLayout"));
}

///////////////////
// IOleInPlaceSite
///////////////////
HRESULT STDMETHODCALLTYPE XcpControlHost::GetWindow(HWND* pHwnd)
{
	//ATLENSURE_RETURN(phwnd);
	//*phwnd = m_hWnd;
	*pHwnd = NULL;
	return E_FAIL;
}
HRESULT STDMETHODCALLTYPE XcpControlHost::ContextSensitiveHelp(BOOL fEnterMode)
{
	ATLTRACENOTIMPL(_T("IOleInPlaceSite::ContextSensitiveHelp"));
}
HRESULT STDMETHODCALLTYPE XcpControlHost::CanInPlaceActivate()
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnInPlaceActivate()
{
	// should only be called once the first time control is inplace-activated
	//ATLASSUME(m_bInPlaceActive == FALSE);
	//ATLASSUME(m_spInPlaceObjectWindowless == NULL);

	//m_bInPlaceActive = TRUE;
	//OleLockRunning(m_spOleObject, TRUE, FALSE);
	//m_bWindowless = FALSE;
	//m_spOleObject->QueryInterface(__uuidof(IOleInPlaceObject), (void**) &m_spInPlaceObjectWindowless);
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnUIActivate()
{
	ATLTRACE(_T("IOleInPlaceSite::OnUIActivate\n"));
	m_UIActive = TRUE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetWindowContext(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo)
{
	ATLTRACE(_T("IOleInPlaceSite::GetWindowContext\n"));
	if (ppFrame != NULL)
		*ppFrame = NULL;
	if (ppDoc != NULL)
		*ppDoc = NULL;

	if (ppFrame == NULL || ppDoc == NULL || lprcPosRect == NULL || lprcClipRect == NULL)
		return E_POINTER;

	pFrameInfo->fMDIApp = FALSE;
	pFrameInfo->haccel = NULL;
	pFrameInfo->cAccelEntries = 0;
	pFrameInfo->hwndFrame = NULL;

	lprcPosRect->top = m_rcPos.top;
	lprcPosRect->left = m_rcPos.left;
	lprcPosRect->right = m_rcPos.right;
	lprcPosRect->bottom = m_rcPos.bottom;

	lprcClipRect->top = m_rcPos.top;
	lprcClipRect->left = m_rcPos.left;
	lprcClipRect->right = m_rcPos.right;
	lprcClipRect->bottom = m_rcPos.bottom;

	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::Scroll(SIZE scrollExtant)
{
	ATLTRACENOTIMPL(_T("IOleInPlaceSite::Scroll"));
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnUIDeactivate(BOOL fUndoable)
{
	ATLTRACE(_T("IOleInPlaceSite::OnUIDeactivate\n"));
	m_UIActive = FALSE;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnInPlaceDeactivate()
{
	ATLTRACE(_T("IOleInPlaceSite::OnInPlaceDeactivate\n"));
	m_bInPlaceActive = FALSE;
	m_spInPlaceObjectWindowless.Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::DiscardUndoState()
{
	ATLTRACE(_T("IOleInPlaceSite::DiscardUndoState"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::DeactivateAndUndo()
{
	ATLTRACE(_T("IOleInPlaceSite::DeactivateAndUndo"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnPosRectChange(LPCRECT lprcPosRect)
{
	ATLTRACE(_T("IOleInPlaceSite::OnPosRectChange"));
	//if (lprcPosRect==NULL) { return E_POINTER; }

	//// Use MoveWindow() to resize the CAxHostWindow.
	//// The CAxHostWindow handler for OnSize() will
	//// take care of calling IOleInPlaceObject::SetObjectRects().

	//// Convert to parent window coordinates for MoveWindow().
	//RECT rect = *lprcPosRect;
	//ClientToScreen( &rect );
	//HWND hWnd = GetParent();

	//// Check to make sure it's a non-top-level window.
	//if(hWnd != NULL)
	//{
	//	CWindow wndParent(hWnd);
	//	wndParent.ScreenToClient(&rect);
	//	wndParent.Detach ();
	//}
	//// Do the actual move.
	//MoveWindow( &rect);
	return S_OK;
}


/////////////////////
// IOleInPlaceSiteEx
/////////////////////
HRESULT STDMETHODCALLTYPE XcpControlHost::OnInPlaceActivateEx(BOOL* pfNoRedraw, DWORD dwFlags)
{
	// should only be called once the first time control is inplace-activated
	ATLTRACE(_T("IOleInPlaceSiteEx::OnInPlaceActivateEx\n"));
	ATLASSERT(m_bInPlaceActive == FALSE);
	ATLASSERT(m_spInPlaceObjectWindowless == NULL);

	m_bInPlaceActive = TRUE;
	OleLockRunning(m_spOleObject, TRUE, FALSE);
	HRESULT hr = E_FAIL;
	if (dwFlags & ACTIVATE_WINDOWLESS)
	{
		hr = m_spOleObject->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (void**) &m_spInPlaceObjectWindowless);

		if (m_spInPlaceObjectWindowless != NULL)
			m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
	}

	return (m_spInPlaceObjectWindowless != NULL) ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnInPlaceDeactivateEx(BOOL fNoRedraw)
{
	ATLTRACE(_T("IOleInPlaceSiteEx::OnInPlaceDeactivateEx\n"));
	m_bInPlaceActive = FALSE;
	m_spInPlaceObjectWindowless.Release();
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::RequestUIActivate()
{
	ATLTRACE(_T("IOleInPlaceSiteEx::RequestUIActivate\n"));
	return S_OK;
}


/////////////////////////////
// IOleInPlaceSiteWindowless
/////////////////////////////
HRESULT STDMETHODCALLTYPE XcpControlHost::CanWindowlessActivate()
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::CanWindowlessActivate\n"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetCapture()
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::GetCapture\n"));
	return m_bCapture ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::SetCapture(BOOL fCapture)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::SetCapture\n"));
	m_bCapture = fCapture;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetFocus()
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::GetFocus\n"));
	return m_bHaveFocus ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::SetFocus(BOOL fGotFocus)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::SetFocus\n"));
	m_bHaveFocus = fGotFocus;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::GetDC"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::ReleaseDC(HDC hDC)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::ReleaseDC"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::InvalidateRect(LPCRECT pRect, BOOL fErase)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::InvalidateRgn(HRGN hRGN, BOOL fErase)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::InvalidateRng\n"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::ScrollRect"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::AdjustRect(LPRECT prc)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::AdjustRect"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnDefWindowMessage(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult)
{
	ATLTRACE(_T("IOleInPlaceSiteWindowless::OnDefWindowMessage"));
	return S_OK;
}

///////////////////
// IOleControlSite
///////////////////
HRESULT STDMETHODCALLTYPE XcpControlHost::OnControlInfoChanged()
{
	ATLTRACE(_T("IOleControlSite::OnControlInfoChanged"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::LockInPlaceActive(BOOL fLock)
{
	ATLTRACE(_T("IOleControlSite::LockInPlaceActive"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::GetExtendedControl(IDispatch** ppDisp)
{
	ATLTRACE(_T("IOleControlSite::GetExtendedControl"));

	if (ppDisp == NULL)
		return E_POINTER;
	return m_spOleObject.QueryInterface(ppDisp);
}

HRESULT STDMETHODCALLTYPE XcpControlHost::TransformCoords(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags)
{
	ATLTRACE(_T("IOleControlSite::TransformCoords"));
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
	ATLTRACE(_T("IOleControlSite::TranslateAccelerator"));
	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::OnFocus(BOOL fGotFocus)
{
	m_bHaveFocus = fGotFocus;
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::ShowPropertyFrame()
{
	ATLTRACE(_T("IOleControlSite::ShowPropertyFrame"));
	return S_OK;
}

///////////////////
// IAdviseSink
///////////////////
void STDMETHODCALLTYPE XcpControlHost::OnDataChange(FORMATETC* pFormatetc, STGMEDIUM* pStgmed)
{
	ATLTRACE(_T("IAdviseSink::OnDataChange\n"));
}

void STDMETHODCALLTYPE XcpControlHost::OnViewChange(DWORD dwAspect, LONG lindex)
{
	ATLTRACE(_T("IAdviseSink::OnViewChange\n"));
}

void STDMETHODCALLTYPE XcpControlHost::OnRename(IMoniker* pmk)
{
	ATLTRACE(_T("IAdviseSink::OnRename\n"));
}

void STDMETHODCALLTYPE XcpControlHost::OnSave()
{
	ATLTRACE(_T("IAdviseSink::OnSave\n"));
}

void STDMETHODCALLTYPE XcpControlHost::OnClose()
{
	ATLTRACE(_T("IAdviseSink::OnClose\n"));
}

// IOleContainer
HRESULT STDMETHODCALLTYPE XcpControlHost::ParseDisplayName(IBindCtx*, LPOLESTR, ULONG*, IMoniker**)
{
	return S_OK;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::EnumObjects(DWORD, IEnumUnknown** ppenum)
{
	if (ppenum == NULL)
		return E_POINTER;
	*ppenum = NULL;
	typedef CComObject<CComEnum<IEnumUnknown, &__uuidof(IEnumUnknown), IUnknown*, _CopyInterface<IUnknown> > > enumunk;
	enumunk* p = NULL;
	ATLTRY(p = new enumunk);
	if(p == NULL)
		return E_OUTOFMEMORY;
	IUnknown* pTemp = m_spUnknown;
	// There is always only one object.
	HRESULT hRes = p->Init(reinterpret_cast<IUnknown**>(&pTemp), reinterpret_cast<IUnknown**>(&pTemp + 1), GetControllingUnknown(), AtlFlagCopy);
	if (SUCCEEDED(hRes))
		hRes = p->QueryInterface(__uuidof(IEnumUnknown), (void**)ppenum);
	if (FAILED(hRes))
		delete p;
	return hRes;
}

HRESULT STDMETHODCALLTYPE XcpControlHost::LockContainer(BOOL)
{
	return S_OK;
}

///////////////////////////////////////////////////////////////////////////////
// IServiceProvider 
///////////////////////////////////////////////////////////////////////////////
STDMETHODIMP XcpControlHost::QueryService(REFGUID rsid, REFIID riid, void** ppvObj) 
{
	ATLASSERT(ppvObj != NULL);
	if (ppvObj == NULL)
		return E_POINTER;
	*ppvObj = NULL;
	
	HRESULT hr = E_NOINTERFACE;

	if (rsid == IID_IXcpControlHost && riid == IID_IXcpControlHost) 
	{
		((IXcpControlHost*)this)->AddRef();
		*ppvObj = (IXcpControlHost*)this;
		hr = S_OK;
	}

	if (rsid == IID_IXcpControlHost2 && riid == IID_IXcpControlHost2) 
	{
		((IXcpControlHost2*)this)->AddRef();
		*ppvObj = (IXcpControlHost2*)this;
		hr = S_OK;
	}

	return hr;
}

void XcpControlHost::SetSize(size_t width, size_t height) 
{
	if(m_spInPlaceObjectWindowless != 0)
	{
		m_rcPos.top = 0;
		m_rcPos.left = 0;
		m_rcPos.right = width;
		m_rcPos.bottom = height;

		m_pxSize.cx = m_rcPos.right - m_rcPos.left;
		m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
		AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
		m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
		m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
		AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
		m_rcPos.right = m_rcPos.left + m_pxSize.cx;
		m_rcPos.bottom = m_rcPos.top + m_pxSize.cy;

		m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
	}
}

HRESULT XcpControlHost::CreateXcpControl() 
{
	HRESULT hr = S_OK;
	
	hr = CoCreateInstance(CLSID_XcpControl, NULL, CLSCTX_INPROC_SERVER, IID_IUnknown, (void**)&m_spUnknown);
		
	if(SUCCEEDED(hr))
	{
		m_spUnknown->QueryInterface(__uuidof(IOleObject), (void**)&m_spOleObject);
		if(m_spOleObject)
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

			//Initialize control
			CComQIPtr<IPersistStreamInit> spPSI(m_spOleObject);
			if (spPSI)
				hr = spPSI->InitNew();

			if (FAILED(hr)) // If the initialization of the control failed...
			{
				// Clean up and return
				if (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
					m_spOleObject->SetClientSite(NULL);

				m_dwMiscStatus = 0;
				m_spOleObject.Release();
				m_spUnknown.Release();

				return hr;
			}
			//end Initialize object

			if (0 == (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST))
			{
				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				m_spOleObject->SetClientSite(spClientSite);
			}
			
			hr = m_spOleObject->QueryInterface(__uuidof(IViewObjectEx), (void**) &m_spViewObject);
			if (FAILED(hr)) 			
				hr = m_spOleObject->QueryInterface(__uuidof(IViewObject2), (void**) &m_spViewObject);	
			if (FAILED(hr)) 			
				hr = m_spOleObject->QueryInterface(__uuidof(IViewObject), (void**) &m_spViewObject);			

			CComQIPtr<IAdviseSink> spAdviseSink(GetControllingUnknown());
			m_spOleObject->Advise(spAdviseSink, &m_dwOleObject);
			if (m_spViewObject)
				m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, spAdviseSink);
			
			m_spOleObject->SetHostNames(OLESTR("AXWIN"), NULL);

			if ((m_dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME) == 0)
			{
				//Initialize window to some dummy size
				m_rcPos.top = 0;
				m_rcPos.left = 0;
				m_rcPos.right = 720;
				m_rcPos.bottom = 576;

				m_pxSize.cx = m_rcPos.right - m_rcPos.left;
				m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
				AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
				m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
				m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
				AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
				m_rcPos.right = m_rcPos.left + m_pxSize.cx;
				m_rcPos.bottom = m_rcPos.top + m_pxSize.cy;

				CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
				hr = m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, NULL, spClientSite, 0, NULL, &m_rcPos);
			}
		}

		CComPtr<IObjectWithSite> spSite;
		m_spUnknown->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
		if (spSite != NULL)
			spSite->SetSite(GetControllingUnknown());
	}

	return hr;
}

HRESULT XcpControlHost::DestroyXcpControl()
{
	if ((!m_spViewObject) == false)
		m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, NULL);

	if ((!m_spOleObject) == false)
	{
		m_spOleObject->Unadvise(m_dwOleObject);
		m_spOleObject->Close(OLECLOSE_NOSAVE);
		m_spOleObject->SetClientSite(NULL);
	}

	if ((!m_spUnknown) == false)
	{
		CComPtr<IObjectWithSite> spSite;
		m_spUnknown->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
		if (spSite != NULL)
			spSite->SetSite(NULL);
	}

	if ((!m_spViewObject) == false)
		m_spViewObject.Release();

	if ((!m_spInPlaceObjectWindowless) == false)
		m_spInPlaceObjectWindowless.Release();

	if ((!m_spOleObject) == false)
		m_spOleObject.Release();

	if ((!m_spUnknown) == false)
		m_spUnknown.Release();

	return S_OK;
}

bool XcpControlHost::DrawControl(HDC targetDC)
{
	DVASPECTINFO aspectInfo = {sizeof(DVASPECTINFO), DVASPECTINFOFLAG_CANOPTIMIZE};
	HRESULT hr = S_OK;

	hr = m_spViewObject->Draw(DVASPECT_CONTENT, -1, &aspectInfo, NULL, NULL, targetDC, NULL, NULL, NULL, NULL); 
	return (hr == S_OK);
}
