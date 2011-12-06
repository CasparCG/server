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
	: public ATL::CAxHostWindow
	, public IXcpControlHost2
{
	HWND hControlWindow;
	IUnknown* m_pUnKnown;
	IXcpControl2* m_pControl;
public: 
	XcpControlHost();
	~XcpControlHost();

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

	
   // IXcpControlHost implementation declarations
	STDMETHOD(GetHostOptions)(DWORD* pdwOptions);
	STDMETHOD(NotifyLoaded)();
	STDMETHOD(NotifyError)(BSTR bstrError, BSTR bstrSource, long nLine, long nColumn);
	STDMETHOD(InvokeHandler)(BSTR bstrName, VARIANT varParam1, VARIANT varParam2, VARIANT* pvarResult);
	STDMETHOD(GetBaseUrl)(BSTR* pbstrUrl);
	STDMETHOD(GetNamedSource)(BSTR bstrSourceID, BSTR* pbstrSource);
	STDMETHOD(DownloadUrl)(BSTR bstrUrl, IXcpControlDownloadCallback* pCallback, IStream** ppStream);
	STDMETHOD(GetCustomAppDomain)(IUnknown** ppAppDomain);
	STDMETHOD(GetControlVersion)(UINT *puMajorVersion, UINT *puMinorVersion);
	
	// Infrastructure for control creation.
	HRESULT CreateXcpControl(HWND hwnd);
	HRESULT DestroyXcpControl();

	DECLARE_NOT_AGGREGATABLE(XcpControlHost);

	BEGIN_COM_MAP(XcpControlHost)
		COM_INTERFACE_ENTRY(IXcpControlHost2)
		COM_INTERFACE_ENTRY_CHAIN(CAxHostWindow)
	END_COM_MAP()
	
	BEGIN_MSG_MAP(XcpControlHost)
	   CHAIN_MSG_MAP(CAxHostWindow)
	END_MSG_MAP()

	//IServiceProvider Implementation
	STDMETHOD(QueryService)(REFGUID rsid, REFIID riid, void** ppvObj);

	// ATL Overrides
	STDMETHOD(AttachControl)(IUnknown* pUnkControl, HWND hWnd);
	HRESULT ActivateXcpControl(IUnknown* pUnkControl);
	
	IXcpControl2* GetXcpControlPtr()
	{
		return m_pControl;
	}
};

