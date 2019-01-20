/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Nicklas P Andersson
 */

#ifndef _FLASHAXCONTAINER_H__
#define _FLASHAXCONTAINER_H__

#pragma once

#include <atlbase.h>
#include <atlcom.h>
#include <atlhost.h>
#include <ocmm.h>

#include <functional>

#include "../interop/axflash.h"
#include <core/video_format.h>
//#import "progid:ShockwaveFlash.ShockwaveFlash.9" no_namespace, named_guids

#include <comdef.h>

#include <InitGuid.h>
#include <ddraw.h>

#ifndef DEFINE_GUID2
#define DEFINE_GUID2(name, l, w1, w2, b1, b2, b3, b4, b5, b6, b7, b8)                                                  \
    const GUID name = {l, w1, w2, {b1, b2, b3, b4, b5, b6, b7, b8}}
#endif

_COM_SMARTPTR_TYPEDEF(IDirectDraw4, IID_IDirectDraw4);

namespace caspar { namespace flash {

class TimerHelper;
/*struct DirtyRect {
    DirtyRect(LONG l, LONG t, LONG r, LONG b, bool e) : bErase(e), bWhole(false) {
        rect.left = l;
        rect.top = t;
        rect.right = r;
        rect.bottom = b;
    }
    DirtyRect(const RECT& rc, bool e) : bErase(e), bWhole(false)  {
        rect.left = rc.left;
        rect.top = rc.top;
        rect.right = rc.right;
        rect.bottom = rc.bottom;
    }
    explicit DirtyRect(bool b) : bWhole(b) {}

    RECT	rect;
    bool	bErase;
    bool	bWhole;
};*/

extern _ATL_FUNC_INFO fnInfoFlashCallEvent;
extern _ATL_FUNC_INFO fnInfoReadyStateChangeEvent;

class ATL_NO_VTABLE FlashAxContainer
    : public ATL::CComCoClass<FlashAxContainer, &CLSID_NULL>
    , public ATL::CComObjectRootEx<ATL::CComMultiThreadModel>
    , public IOleClientSite
    , public IOleContainer
    , public IOleControlSite
    , public IOleInPlaceSiteWindowless
    , public IObjectWithSiteImpl<FlashAxContainer>
    , public IServiceProvider
    , public IAdviseSink
    , public ITimerService
    , public ITimer
    , public IDispatchImpl<IDispatch>
    , public IDispEventSimpleImpl<0, FlashAxContainer, &DIID__IShockwaveFlashEvents>
{
  public:
    FlashAxContainer();
    virtual ~FlashAxContainer();

    DECLARE_NO_REGISTRY()
    DECLARE_POLY_AGGREGATABLE(FlashAxContainer)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(FlashAxContainer)
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

    COM_INTERFACE_ENTRY(ITimerService)

    COM_INTERFACE_ENTRY(ITimer)
    END_COM_MAP()

    BEGIN_SINK_MAP(FlashAxContainer)
    SINK_ENTRY_INFO(0, DIID__IShockwaveFlashEvents, static_cast<DISPID>(0xc5), OnFlashCall, &fnInfoFlashCallEvent)
    SINK_ENTRY_INFO(0,
                    DIID__IShockwaveFlashEvents,
                    static_cast<DISPID>(0xfffffd9f),
                    OnReadyStateChange,
                    &fnInfoReadyStateChangeEvent)
    END_SINK_MAP()

    void STDMETHODCALLTYPE OnFlashCall(BSTR request);
    void STDMETHODCALLTYPE OnReadyStateChange(long newState);

    // IObjectWithSite
    STDMETHOD(SetSite)(IUnknown* pUnkSite) override;

    // IOleClientSite
    STDMETHOD(SaveObject)() override;
    STDMETHOD(GetMoniker)(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk) override;
    STDMETHOD(GetContainer)(IOleContainer** ppContainer) override;
    STDMETHOD(ShowObject)() override;
    STDMETHOD(OnShowWindow)(BOOL fShow) override;
    STDMETHOD(RequestNewObjectLayout)() override;

    // IOleInPlaceSite
    STDMETHOD(GetWindow)(HWND* pHwnd) override;
    STDMETHOD(ContextSensitiveHelp)(BOOL fEnterMode) override;
    STDMETHOD(CanInPlaceActivate)() override;
    STDMETHOD(OnInPlaceActivate)() override;
    STDMETHOD(OnInPlaceDeactivate)() override;
    STDMETHOD(OnUIActivate)() override;
    STDMETHOD(OnUIDeactivate)(BOOL fUndoable) override;
    STDMETHOD(GetWindowContext)
    (IOleInPlaceFrame**    ppFrame,
     IOleInPlaceUIWindow** ppDoc,
     LPRECT                lprcPosRect,
     LPRECT                lprcClipRect,
     LPOLEINPLACEFRAMEINFO pFrameInfo) override;
    STDMETHOD(Scroll)(SIZE scrollExtant) override;
    STDMETHOD(DiscardUndoState)() override;
    STDMETHOD(DeactivateAndUndo)() override;
    STDMETHOD(OnPosRectChange)(LPCRECT lprcPosRect) override;

    // IOleInPlaceSiteEx
    STDMETHOD(OnInPlaceActivateEx)(BOOL* pfNoRedraw, DWORD dwFlags) override;
    STDMETHOD(OnInPlaceDeactivateEx)(BOOL fNoRedraw) override;
    STDMETHOD(RequestUIActivate)() override;

    // IOleInPlaceSiteWindowless
    STDMETHOD(CanWindowlessActivate)() override;
    STDMETHOD(GetCapture)() override;
    STDMETHOD(SetCapture)(BOOL fCapture) override;
    STDMETHOD(GetFocus)() override;
    STDMETHOD(SetFocus)(BOOL fGotFocus) override;
    STDMETHOD(GetDC)(LPCRECT pRect, DWORD grfFlags, HDC* phDC) override;
    STDMETHOD(ReleaseDC)(HDC hDC) override;
    STDMETHOD(InvalidateRect)(LPCRECT pRect, BOOL fErase) override;
    STDMETHOD(InvalidateRgn)(HRGN hRGN, BOOL fErase) override;
    STDMETHOD(ScrollRect)(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip) override;
    STDMETHOD(AdjustRect)(LPRECT prc) override;
    STDMETHOD(OnDefWindowMessage)(UINT msg, WPARAM wParam, LPARAM lParam, LRESULT* plResult) override;

    // IOleControlSite
    STDMETHOD(OnControlInfoChanged)() override;
    STDMETHOD(LockInPlaceActive)(BOOL fLock) override;
    STDMETHOD(GetExtendedControl)(IDispatch** ppDisp) override;
    STDMETHOD(TransformCoords)(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags) override;
    STDMETHOD(TranslateAccelerator)(LPMSG lpMsg, DWORD grfModifiers);
    STDMETHOD(OnFocus)(BOOL fGotFocus) override;
    STDMETHOD(ShowPropertyFrame)() override;

    // IAdviseSink
    STDMETHOD_(void, OnDataChange)(FORMATETC* pFormatetc, STGMEDIUM* pStgmed) override;
    STDMETHOD_(void, OnViewChange)(DWORD dwAspect, LONG lindex) override;
    STDMETHOD_(void, OnRename)(IMoniker* pmk) override;
    STDMETHOD_(void, OnSave)() override;
    STDMETHOD_(void, OnClose)() override;

    // IServiceProvider
    STDMETHOD(QueryService)(REFGUID rsid, REFIID riid, void** ppvObj) override;

    // IOleContainer
    STDMETHOD(ParseDisplayName)(IBindCtx*, LPOLESTR, ULONG*, IMoniker**) override
    {
        ATLTRACENOTIMPL(_T("IOleContainer::ParseDisplayName"));
    }
    STDMETHOD(EnumObjects)(DWORD, IEnumUnknown** ppenum) override
    {
        if (ppenum == nullptr)
            return E_POINTER;
        *ppenum = nullptr;
        using enumunk =
            CComObject<CComEnum<IEnumUnknown, &__uuidof(IEnumUnknown), IUnknown*, _CopyInterface<IUnknown>>>;
        enumunk* p = nullptr;
        ATLTRY(p = new enumunk);
        if (p == nullptr)
            return E_OUTOFMEMORY;
        IUnknown* pTemp = m_spUnknown;
        // There is always only one object.
        HRESULT hRes = p->Init(reinterpret_cast<IUnknown**>(&pTemp),
                               reinterpret_cast<IUnknown**>(&pTemp + 1),
                               GetControllingUnknown(),
                               AtlFlagCopy);
        if (SUCCEEDED(hRes))
            hRes = p->QueryInterface(__uuidof(IEnumUnknown), (void**)ppenum);
        if (FAILED(hRes))
            delete p;
        return hRes;
    }
    STDMETHOD(LockContainer)(BOOL) override { ATLTRACENOTIMPL(_T("IOleContainer::LockContainer")); }

    // ITimerService
    STDMETHOD(CreateTimer)(ITimer* pReferenceTimer, ITimer** ppNewTimer) override;
    STDMETHOD(GetNamedTimer)(REFGUID rguidName, ITimer** ppTimer) override;
    STDMETHOD(SetNamedTimerReference)(REFGUID rguidName, ITimer* pReferenceTimer) override;

    // ITimer
    STDMETHOD(Advise)
    (VARIANT vtimeMin, VARIANT vtimeMax, VARIANT vtimeInterval, DWORD dwFlags, ITimerSink* pTimerSink, DWORD* pdwCookie)
        override;
    STDMETHOD(Unadvise)(DWORD dwCookie) override;
    STDMETHOD(Freeze)(BOOL fFreeze) override;
    STDMETHOD(GetTime)(VARIANT* pvtime) override;
    double GetFPS();

    void set_print(const std::function<std::wstring()>& print) { print_ = print; }

    HRESULT
    CreateAxControl();
    void DestroyAxControl();
    HRESULT
    QueryControl(REFIID iid, void** ppUnk);

    template <class Q>
    HRESULT QueryControl(Q** ppUnk)
    {
        return QueryControl(__uuidof(Q), (void**)ppUnk);
    }

    //	static ATL::CComObject<FlashAxContainer>* CreateInstance();

    void Tick();
    bool FlashCall(const std::wstring& str, std::wstring& result);
    bool DrawControl(HDC targetDC);
    bool InvalidRect() const { return bInvalidRect_; }
    bool IsEmpty() const { return bIsEmpty_; }

    void SetSize(size_t width, size_t height);
    bool IsReadyToRender() const;
    void EnterFullscreen();

    static bool CheckForFlashSupport();

    ATL::CComPtr<IOleInPlaceObjectWindowless> m_spInPlaceObjectWindowless;

  private:
    std::function<std::wstring()> print_;
    TimerHelper*                  pTimerHelper;
    volatile bool                 bInvalidRect_;
    volatile bool                 bCallSuccessful_;
    volatile bool                 bReadyToRender_;
    volatile bool                 bIsEmpty_;
    volatile bool                 bHasNewTiming_;
    // std::vector<DirtyRect> bDirtyRects_;

    IDirectDraw4Ptr* m_lpDD4;
    static CComBSTR  flashGUID_;

    DWORD timerCount_;

    //	state
    bool          bUIActive_;
    bool          bInPlaceActive_;
    unsigned long bHaveFocus_ : 1;
    unsigned long bCapture_ : 1;

    DWORD m_dwOleObject;
    DWORD m_dwMiscStatus;
    SIZEL m_hmSize;
    SIZEL m_pxSize;
    RECT  m_rcPos;

    ATL::CComPtr<IUnknown>      m_spUnknown;
    ATL::CComPtr<IOleObject>    m_spServices;
    ATL::CComPtr<IOleObject>    m_spOleObject;
    ATL::CComPtr<IViewObjectEx> m_spViewObject;

    //	ATL::CComPtr<ATL::CComObject<MyMoniker> > m_spMyMoniker;
};

}} // namespace caspar::flash

#endif //_FLASHAXCONTAINER_H__
