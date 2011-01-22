// XcpControlHost.h : Declaration of the XcpControlHost

#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <atlctl.h>

#include "xcpctrl_h.h"

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

class XcpControlHost 
	: public IXcpControlHost2
	, public ATL::CComCoClass<XcpControlHost , &CLSID_NULL>
	, public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>
	, public IOleClientSite
	, public IOleContainer
	, public IOleControlSite
	, public IOleInPlaceSiteWindowless
	, public IObjectWithSiteImpl<XcpControlHost>
	, public IServiceProvider
	, public IAdviseSink
	, public IDispatchImpl<IDispatch>
{
	ATL::CComPtr<IXcpControl2> m_pControl;
	
	ATL::CComPtr<IOleInPlaceObjectWindowless> m_spInPlaceObjectWindowless;
	ATL::CComPtr<IUnknown> m_spUnknown;
	ATL::CComPtr<IOleObject> m_spServices;
	ATL::CComPtr<IOleObject> m_spOleObject;
	ATL::CComPtr<IViewObjectEx> m_spViewObject;
	
	DWORD m_dwOleObject;
	DWORD m_dwMiscStatus;
	SIZEL m_hmSize;
	SIZEL m_pxSize;
	RECT m_rcPos;
	
	BOOL m_UIActive;
	BOOL m_bInPlaceActive;
	BOOL m_bHaveFocus;
	BOOL m_bCapture;
public: 
	typedef enum
	{
		XcpHostOption_FreezeOnInitialFrame       = 0x001,
		XcpHostOption_DisableFullScreen          = 0x002,
		XcpHostOption_DisableManagedExecution    = 0x008,
		XcpHostOption_EnableCrossDomainDownloads = 0x010,
		XcpHostOption_UseCustomAppDomain         = 0x020,
		XcpHostOption_DisableNetworking          = 0x040,        
		XcpHostOption_DisableScriptCallouts      = 0x080,
		XcpHostOption_EnableHtmlDomAccess        = 0x100,
		XcpHostOption_EnableScriptableObjectAccess = 0x200,
	} XcpHostOptions;

	XcpControlHost();
	~XcpControlHost();	
	
	IXcpControl2* GetXcpControlPtr(){ return m_pControl; }
		
	DECLARE_NO_REGISTRY()
	DECLARE_POLY_AGGREGATABLE(XcpControlHost)
	DECLARE_GET_CONTROLLING_UNKNOWN()

	BEGIN_COM_MAP(XcpControlHost)
		COM_INTERFACE_ENTRY(IXcpControlHost2)
		
		COM_INTERFACE_ENTRY(IDispatch)
		COM_INTERFACE_ENTRY(IOleClientSite)
		COM_INTERFACE_ENTRY(IObjectWithSite)
		COM_INTERFACE_ENTRY(IOleControlSite)
		COM_INTERFACE_ENTRY(IOleContainer)

		COM_INTERFACE_ENTRY(IOleInPlaceSiteWindowless)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteEx)
		COM_INTERFACE_ENTRY(IOleInPlaceSite)
		COM_INTERFACE_ENTRY(IOleWindow)

		COM_INTERFACE_ENTRY(IServiceProvider)

		COM_INTERFACE_ENTRY(IAdviseSink)
	END_COM_MAP()

// IXcpControlHost
	STDMETHOD(GetHostOptions)(DWORD* pdwOptions);
	STDMETHOD(NotifyLoaded)();
	STDMETHOD(NotifyError)(BSTR bstrError, BSTR bstrSource, long nLine, long nColumn);
	STDMETHOD(InvokeHandler)(BSTR bstrName, VARIANT varParam1, VARIANT varParam2, VARIANT* pvarResult);
	STDMETHOD(GetBaseUrl)(BSTR* pbstrUrl);
	STDMETHOD(GetNamedSource)(BSTR bstrSourceID, BSTR* pbstrSource);
	STDMETHOD(DownloadUrl)(BSTR bstrUrl, IXcpControlDownloadCallback* pCallback, IStream** ppStream);
	STDMETHOD(GetCustomAppDomain)(IUnknown** ppAppDomain);
	STDMETHOD(GetControlVersion)(UINT *puMajorVersion, UINT *puMinorVersion);
		
// IObjectWithSite
	STDMETHOD(SetSite)(IUnknown* pUnkSite);

// IOleClientSite
	STDMETHOD(SaveObject)();
	STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk);
	STDMETHOD(GetContainer)(IOleContainer** ppContainer);
	STDMETHOD(ShowObject)();
	STDMETHOD(OnShowWindow)(BOOL fShow);
	STDMETHOD(RequestNewObjectLayout)();

// IOleInPlaceSite
	STDMETHOD(GetWindow)(HWND* pHwnd);
	STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode);
	STDMETHOD(CanInPlaceActivate)();
	STDMETHOD(OnInPlaceActivate)();
	STDMETHOD(OnInPlaceDeactivate)();
	STDMETHOD(OnUIActivate)();
	STDMETHOD(OnUIDeactivate)(BOOL fUndoable);
	STDMETHOD(GetWindowContext)(IOleInPlaceFrame** ppFrame, IOleInPlaceUIWindow** ppDoc, LPRECT lprcPosRect, LPRECT lprcClipRect, LPOLEINPLACEFRAMEINFO pFrameInfo);
	STDMETHOD(Scroll)(SIZE scrollExtant);
	STDMETHOD(DiscardUndoState)();
	STDMETHOD(DeactivateAndUndo)();
	STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect);

// IOleInPlaceSiteEx
	STDMETHOD(OnInPlaceActivateEx)(BOOL* pfNoRedraw, DWORD dwFlags);
	STDMETHOD(OnInPlaceDeactivateEx)(BOOL fNoRedraw);
	STDMETHOD(RequestUIActivate)();

// IOleInPlaceSiteWindowless
	STDMETHOD(CanWindowlessActivate)();
	STDMETHOD(GetCapture)();
	STDMETHOD(SetCapture)(BOOL fCapture);
	STDMETHOD(GetFocus)();
	STDMETHOD(SetFocus)(BOOL fGotFocus);
	STDMETHOD(GetDC)(LPCRECT pRect, DWORD grfFlags, HDC* phDC);
	STDMETHOD(ReleaseDC)(HDC hDC);
	STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase);
	STDMETHOD(InvalidateRgn)(HRGN hRGN, BOOL fErase);
	STDMETHOD(ScrollRect)(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip);
	STDMETHOD(AdjustRect)(LPRECT prc);
	STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult);

// IOleControlSite
	STDMETHOD(OnControlInfoChanged)();
	STDMETHOD(LockInPlaceActive)(BOOL fLock);
	STDMETHOD(GetExtendedControl)(IDispatch** ppDisp);
	STDMETHOD(TransformCoords)(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags);
	STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, DWORD grfModifiers);
	STDMETHOD(OnFocus)(BOOL fGotFocus);
	STDMETHOD(ShowPropertyFrame)();

// IAdviseSink
	STDMETHOD_(void, OnDataChange)(FORMATETC* pFormatetc, STGMEDIUM* pStgmed);
	STDMETHOD_(void, OnViewChange)(DWORD dwAspect, LONG lindex);
	STDMETHOD_(void, OnRename)(IMoniker* pmk);
	STDMETHOD_(void, OnSave)();
	STDMETHOD_(void, OnClose)();

// IServiceProvider
	STDMETHOD(QueryService)( REFGUID rsid, REFIID riid, void** ppvObj);

// IOleContainer
	STDMETHOD(ParseDisplayName)(IBindCtx*, LPOLESTR, ULONG*, IMoniker**);
	STDMETHOD(EnumObjects)(DWORD, IEnumUnknown** ppenum);
	STDMETHOD(LockContainer)(BOOL);

// Misc
	void SetSize(size_t width, size_t height);

	HRESULT CreateXcpControl();
	HRESULT DestroyXcpControl();

	bool DrawControl(HDC targetDC);
};

