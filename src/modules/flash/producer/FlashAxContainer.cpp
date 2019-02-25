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

#include "../stdafx.h"

#include "../interop/TimerHelper.h"
#include "FlashAxContainer.h"

#include <common/log.h>

#if defined(_MSC_VER)
#pragma warning(push, 2) // TODO
#endif

using namespace ATL;

namespace caspar { namespace flash {

CComBSTR FlashAxContainer::flashGUID_(_T("{D27CDB6E-AE6D-11CF-96B8-444553540000}"));

_ATL_FUNC_INFO fnInfoFlashCallEvent        = {CC_STDCALL, VT_EMPTY, 1, {VT_BSTR}};
_ATL_FUNC_INFO fnInfoReadyStateChangeEvent = {CC_STDCALL, VT_EMPTY, 1, {VT_I4}};

FlashAxContainer::FlashAxContainer()
    : pTimerHelper(nullptr)
    , bInvalidRect_(false)
    , bReadyToRender_(false)
    , bIsEmpty_(true)
    , bHasNewTiming_(false)
    , m_lpDD4(nullptr)
    , timerCount_(0)
    , bInPlaceActive_(FALSE)
{
}
FlashAxContainer::~FlashAxContainer()
{
    if (m_lpDD4) {
        m_lpDD4->Release();
        delete m_lpDD4;
    }

    if (pTimerHelper != nullptr)
        delete pTimerHelper;
}

/////////
// IObjectWithSite
/////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::SetSite(IUnknown* pUnkSite)
{
    ATLTRACE(_T("IObjectWithSite::SetSite\n"));
    HRESULT hr = IObjectWithSiteImpl<FlashAxContainer>::SetSite(pUnkSite);

    if (SUCCEEDED(hr) && m_spUnkSite) {
        // Look for "outer" IServiceProvider
        hr = m_spUnkSite->QueryInterface(__uuidof(IServiceProvider), (void**)&m_spServices);
        ATLASSERT(!hr && _T("No ServiceProvider!"));
    }

    if (pUnkSite == nullptr)
        m_spServices.Release();

    return hr;
}

/////////
// IOleClientSite
/////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::SaveObject() { ATLTRACENOTIMPL(_T("IOleClientSite::SaveObject")); }

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetMoniker(DWORD dwAssign, DWORD dwWhichMoniker, IMoniker** ppmk)
{
    /*	if(*ppmk != NULL) {
            if(m_spMyMoniker == NULL) {
                ATL::CComObject<MyMoniker>* pMoniker = NULL;
                HRESULT hr = ATL::CComObject<MyMoniker>::CreateInstance(&pMoniker);
                if(SUCCEEDED(hr))
                    m_spMyMoniker = pMoniker;
            }

            if(m_spMyMoniker != NULL) {
                *ppmk = m_spMyMoniker;
                (*ppmk)->AddRef();
                return S_OK;
            }
        }
    */
    if (ppmk != nullptr)
        *ppmk = nullptr;
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetContainer(IOleContainer** ppContainer)
{
    ATLTRACE(_T("IOleClientSite::GetContainer\n"));
    *ppContainer = nullptr;
    return E_NOINTERFACE;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::ShowObject()
{
    ATLTRACE(_T("IOleClientSite::ShowObject\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnShowWindow(BOOL fShow)
{
    ATLTRACE(_T("IOleClientSite::OnShowWindow"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::RequestNewObjectLayout()
{
    ATLTRACE(_T("IOleClientSite::RequestNewObjectLayout"));
    return S_OK;
}

/////////
// IOleInPlaceSite
/////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::GetWindow(HWND* pHwnd)
{
    ATLTRACE(_T("IOleInPlaceSite::GetWindow\n"));
    *pHwnd = nullptr; // GetApplication()->GetMainWindow()->getHwnd();
    return E_FAIL;
}
HRESULT STDMETHODCALLTYPE FlashAxContainer::ContextSensitiveHelp(BOOL fEnterMode)
{
    ATLTRACE(_T("IOleInPlaceSite::ContextSensitiveHelp"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::CanInPlaceActivate()
{
    ATLTRACE(_T("IOleInPlaceSite::CanInPlaceActivate\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnInPlaceActivate()
{
    ATLTRACE(_T("IOleInPlaceSite::OnInPlaceActivate"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnUIActivate()
{
    ATLTRACE(_T("IOleInPlaceSite::OnUIActivate\n"));
    bUIActive_ = TRUE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetWindowContext(IOleInPlaceFrame**    ppFrame,
                                                             IOleInPlaceUIWindow** ppDoc,
                                                             LPRECT                lprcPosRect,
                                                             LPRECT                lprcClipRect,
                                                             LPOLEINPLACEFRAMEINFO pFrameInfo)
{
    ATLTRACE(_T("IOleInPlaceSite::GetWindowContext\n"));
    if (ppFrame != nullptr)
        *ppFrame = nullptr;
    if (ppDoc != nullptr)
        *ppDoc = nullptr;

    if (ppFrame == nullptr || ppDoc == nullptr || lprcPosRect == nullptr || lprcClipRect == nullptr)
        return E_POINTER;

    pFrameInfo->fMDIApp       = FALSE;
    pFrameInfo->haccel        = nullptr;
    pFrameInfo->cAccelEntries = 0;
    pFrameInfo->hwndFrame     = nullptr;

    lprcPosRect->top    = m_rcPos.top;
    lprcPosRect->left   = m_rcPos.left;
    lprcPosRect->right  = m_rcPos.right;
    lprcPosRect->bottom = m_rcPos.bottom;

    lprcClipRect->top    = m_rcPos.top;
    lprcClipRect->left   = m_rcPos.left;
    lprcClipRect->right  = m_rcPos.right;
    lprcClipRect->bottom = m_rcPos.bottom;

    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::Scroll(SIZE scrollExtant)
{
    ATLTRACE(_T("IOleInPlaceSite::Scroll"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnUIDeactivate(BOOL fUndoable)
{
    ATLTRACE(_T("IOleInPlaceSite::OnUIDeactivate\n"));
    bUIActive_ = FALSE;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnInPlaceDeactivate()
{
    ATLTRACE(_T("IOleInPlaceSite::OnInPlaceDeactivate\n"));
    bInPlaceActive_ = FALSE;
    m_spInPlaceObjectWindowless.Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::DiscardUndoState()
{
    ATLTRACE(_T("IOleInPlaceSite::DiscardUndoState"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::DeactivateAndUndo()
{
    ATLTRACE(_T("IOleInPlaceSite::DeactivateAndUndo"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnPosRectChange(LPCRECT lprcPosRect)
{
    ATLTRACE(_T("IOleInPlaceSite::OnPosRectChange"));
    return S_OK;
}

//////////
// IOleInPlaceSiteEx
//////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::OnInPlaceActivateEx(BOOL* pfNoRedraw, DWORD dwFlags)
{
    // should only be called once the first time control is inplace-activated
    ATLTRACE(_T("IOleInPlaceSiteEx::OnInPlaceActivateEx\n"));
    ATLASSERT(bInPlaceActive_ == FALSE);
    ATLASSERT(m_spInPlaceObjectWindowless == NULL);

    bInPlaceActive_ = TRUE;
    OleLockRunning(m_spOleObject, TRUE, FALSE);
    HRESULT hr = E_FAIL;
    if (dwFlags & ACTIVATE_WINDOWLESS) {
        hr = m_spOleObject->QueryInterface(__uuidof(IOleInPlaceObjectWindowless), (void**)&m_spInPlaceObjectWindowless);

        if (m_spInPlaceObjectWindowless != nullptr)
            m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
    }

    return m_spInPlaceObjectWindowless != nullptr ? S_OK : E_FAIL;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnInPlaceDeactivateEx(BOOL fNoRedraw)
{
    ATLTRACE(_T("IOleInPlaceSiteEx::OnInPlaceDeactivateEx\n"));
    bInPlaceActive_ = FALSE;
    m_spInPlaceObjectWindowless.Release();
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::RequestUIActivate()
{
    ATLTRACE(_T("IOleInPlaceSiteEx::RequestUIActivate\n"));
    return S_OK;
}

//////////////
// IOleInPlaceSiteWindowless
//////////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::CanWindowlessActivate()
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::CanWindowlessActivate\n"));
    return S_OK;
    //	return S_FALSE;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetCapture()
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::GetCapture\n"));
    return bCapture_ ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::SetCapture(BOOL fCapture)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::SetCapture\n"));
    bCapture_ = fCapture;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetFocus()
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::GetFocus\n"));
    return bHaveFocus_ ? S_OK : S_FALSE;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::SetFocus(BOOL fGotFocus)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::SetFocus\n"));
    bHaveFocus_ = fGotFocus;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetDC(LPCRECT pRect, DWORD grfFlags, HDC* phDC)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::GetDC"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::ReleaseDC(HDC hDC)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::ReleaseDC"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::InvalidateRect(LPCRECT pRect, BOOL fErase)
{
    //	ATLTRACE(_T("IOleInPlaceSiteWindowless::InvalidateRect\n"));

    bInvalidRect_ = true;

    /*	//Keep a list of dirty rectangles in order to be able to redraw only them
        if(pRect != NULL) {
            bDirtyRects_.push_back(DirtyRect(*pRect, fErase != 0));
        }
        else {
            bDirtyRects_.push_back(DirtyRect(true));
        }
    */
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::InvalidateRgn(HRGN hRGN, BOOL fErase)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::InvalidateRng\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::ScrollRect(INT dx, INT dy, LPCRECT pRectScroll, LPCRECT pRectClip)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::ScrollRect"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::AdjustRect(LPRECT prc)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::AdjustRect"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnDefWindowMessage(UINT     msg,
                                                               WPARAM   wParam,
                                                               LPARAM   lParam,
                                                               LRESULT* plResult)
{
    ATLTRACE(_T("IOleInPlaceSiteWindowless::OnDefWindowMessage"));
    return S_OK;
}

/////////
// IOleControlSite
/////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::OnControlInfoChanged()
{
    ATLTRACE(_T("IOleControlSite::OnControlInfoChanged"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::LockInPlaceActive(BOOL fLock)
{
    ATLTRACE(_T("IOleControlSite::LockInPlaceActive"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetExtendedControl(IDispatch** ppDisp)
{
    ATLTRACE(_T("IOleControlSite::GetExtendedControl"));

    if (ppDisp == nullptr)
        return E_POINTER;
    return m_spOleObject.QueryInterface(ppDisp);
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::TransformCoords(POINTL* pPtlHimetric, POINTF* pPtfContainer, DWORD dwFlags)
{
    ATLTRACE(_T("IOleControlSite::TransformCoords"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::TranslateAccelerator(LPMSG lpMsg, DWORD grfModifiers)
{
    ATLTRACE(_T("IOleControlSite::TranslateAccelerator"));
    return S_FALSE;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::OnFocus(BOOL fGotFocus)
{
    bHaveFocus_ = fGotFocus;
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::ShowPropertyFrame()
{
    ATLTRACE(_T("IOleControlSite::ShowPropertyFrame"));
    return S_OK;
}

/////////
// IAdviseSink
/////////
void STDMETHODCALLTYPE FlashAxContainer::OnDataChange(FORMATETC* pFormatetc, STGMEDIUM* pStgmed)
{
    ATLTRACE(_T("IAdviseSink::OnDataChange\n"));
}

void STDMETHODCALLTYPE FlashAxContainer::OnViewChange(DWORD dwAspect, LONG lindex)
{
    ATLTRACE(_T("IAdviseSink::OnViewChange\n"));
}

void STDMETHODCALLTYPE FlashAxContainer::OnRename(IMoniker* pmk) { ATLTRACE(_T("IAdviseSink::OnRename\n")); }

void STDMETHODCALLTYPE FlashAxContainer::OnSave() { ATLTRACE(_T("IAdviseSink::OnSave\n")); }

void STDMETHODCALLTYPE FlashAxContainer::OnClose() { ATLTRACE(_T("IAdviseSink::OnClose\n")); }

// DirectDraw GUIDS

DEFINE_GUID2(CLSID_DirectDraw, 0xD7B70EE0, 0x4340, 0x11CF, 0xB0, 0x63, 0x00, 0x20, 0xAF, 0xC2, 0xCD, 0x35);
DEFINE_GUID2(CLSID_DirectDraw7, 0x3c305196, 0x50db, 0x11d3, 0x9c, 0xfe, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc5);

DEFINE_GUID2(IID_IDirectDraw, 0x6C14DB80, 0xA733, 0x11CE, 0xA5, 0x21, 0x00, 0x20, 0xAF, 0x0B, 0xE5, 0x60);
DEFINE_GUID2(IID_IDirectDraw3, 0x618f8ad4, 0x8b7a, 0x11d0, 0x8f, 0xcc, 0x0, 0xc0, 0x4f, 0xd9, 0x18, 0x9d);
DEFINE_GUID2(IID_IDirectDraw4, 0x9c59509a, 0x39bd, 0x11d1, 0x8c, 0x4a, 0x00, 0xc0, 0x4f, 0xd9, 0x30, 0xc5);
DEFINE_GUID2(IID_IDirectDraw7, 0x15e65ec0, 0x3b9c, 0x11d2, 0xb9, 0x2f, 0x00, 0x60, 0x97, 0x97, 0xea, 0x5b);

/////////
// IServiceProvider
/////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::QueryService(REFGUID rsid, REFIID riid, void** ppvObj)
{
    //	ATLTRACE(_T("IServiceProvider::QueryService\n"));
    // the flashcontrol asks for an interface {618F8AD4-8B7A-11D0-8FCC-00C04FD9189D}, this is IID for a DirectDraw3
    // object

    ATLASSERT(ppvObj != NULL);
    if (ppvObj == nullptr)
        return E_POINTER;
    *ppvObj = nullptr;

    HRESULT hr;
    // Author: Makarov Igor
    // Transparent Flash Control in Plain C++
    // http://www.codeproject.com/KB/COM/flashcontrol.aspx
    if (IsEqualGUID(rsid, IID_IDirectDraw3)) {
        if (!m_lpDD4) {
            m_lpDD4 = new IDirectDraw4Ptr;
            hr      = m_lpDD4->CreateInstance(CLSID_DirectDraw, nullptr, CLSCTX_INPROC_SERVER);
            if (FAILED(hr)) {
                delete m_lpDD4;
                m_lpDD4 = nullptr;
                CASPAR_LOG(info) << print_() << " DirectDraw not installed. Running without DirectDraw.";
                return E_NOINTERFACE;
            }
        }
        if (m_lpDD4 && m_lpDD4->GetInterfacePtr()) {
            *ppvObj = m_lpDD4->GetInterfacePtr();
            m_lpDD4->AddRef();
            return S_OK;
        }
    }

    // TODO: The fullscreen-consumer requires that ths does NOT return an ITimerService
    hr = QueryInterface(riid, ppvObj); // E_NOINTERFACE;

    return hr;
}

/////////
// ITimerService
/////////
HRESULT STDMETHODCALLTYPE FlashAxContainer::CreateTimer(ITimer* pReferenceTimer, ITimer** ppNewTimer)
{
    ATLTRACE(_T("ITimerService::CreateTimer\n"));
    if (pTimerHelper != nullptr) {
        delete pTimerHelper;
        pTimerHelper = nullptr;
    }
    pTimerHelper = new TimerHelper();
    return QueryInterface(__uuidof(ITimer), (void**)ppNewTimer);
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetNamedTimer(REFGUID rguidName, ITimer** ppTimer)
{
    ATLTRACE(_T("ITimerService::GetNamedTimer"));
    if (ppTimer == nullptr)
        return E_POINTER;
    *ppTimer = nullptr;

    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::SetNamedTimerReference(REFGUID rguidName, ITimer* pReferenceTimer)
{
    ATLTRACE(_T("ITimerService::SetNamedTimerReference"));
    return S_OK;
}

//////
// ITimer
//////
HRESULT STDMETHODCALLTYPE FlashAxContainer::Advise(VARIANT     vtimeMin,
                                                   VARIANT     vtimeMax,
                                                   VARIANT     vtimeInterval,
                                                   DWORD       dwFlags,
                                                   ITimerSink* pTimerSink,
                                                   DWORD*      pdwCookie)
{
    ATLTRACE(_T("Timer::Advise\n"));

    if (pdwCookie == nullptr)
        return E_POINTER;

    if (pTimerHelper != nullptr) {
        // static tbb::atomic<DWORD> NEXT_ID;
        pTimerHelper->Setup(0, vtimeMin.ulVal, vtimeInterval.ulVal, pTimerSink);
        *pdwCookie     = pTimerHelper->ID;
        bHasNewTiming_ = true;

        return S_OK;
    }
    return E_OUTOFMEMORY;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::Unadvise(/* [in] */ DWORD dwCookie)
{
    ATLTRACE(_T("Timer::Unadvice\n"));
    if (pTimerHelper != nullptr) {
        pTimerHelper->pTimerSink = nullptr;
        return S_OK;
    }
    return E_FAIL;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::Freeze(/* [in] */ BOOL fFreeze)
{
    ATLTRACE(_T("Timer::Freeze\n"));
    return S_OK;
}

HRESULT STDMETHODCALLTYPE FlashAxContainer::GetTime(/* [out] */ VARIANT* pvtime)
{
    ATLTRACE(_T("Timer::GetTime\n"));
    if (pvtime == nullptr)
        return E_POINTER;

    //	return E_NOTIMPL;
    pvtime->lVal = 0;
    return S_OK;
}

double FlashAxContainer::GetFPS()
{
    if (pTimerHelper != nullptr && pTimerHelper->interval > 0)
        return 1000.0 / static_cast<double>(pTimerHelper->interval);

    return 0.0;
}

bool FlashAxContainer::IsReadyToRender() const { return bReadyToRender_; }

void FlashAxContainer::EnterFullscreen()
{
    if (m_spInPlaceObjectWindowless != nullptr) {
        LRESULT result;
        m_spInPlaceObjectWindowless->OnWindowMessage(WM_LBUTTONDOWN, 0, MAKELPARAM(1, 1), &result);
        m_spInPlaceObjectWindowless->OnWindowMessage(WM_LBUTTONUP, 0, MAKELPARAM(1, 1), &result);
    }
}

void STDMETHODCALLTYPE FlashAxContainer::OnFlashCall(BSTR request)
{
    std::wstring str(request);
    if (str.find(L"DisplayedTemplate") != std::wstring::npos) {
        ATLTRACE(_T("ShockwaveFlash::DisplayedTemplate\n"));
        bReadyToRender_ = true;
    } else if (str.find(L"OnCommand") != std::wstring::npos) {
        // this is how templatehost 1.8 reports that a command has been received
        CASPAR_LOG(debug) << print_() << L" [command]      " << str;
        bCallSuccessful_ = true;
    } else if (str.find(L"Activity") != std::wstring::npos) {
        CASPAR_LOG(debug) << print_() << L" [activity]     " << str;

        // this is how templatehost 1.7 reports that a command has been received
        if (str.find(L"Command recieved") != std::wstring::npos || str.find(L"Command received") != std::wstring::npos)
            bCallSuccessful_ = true;

        /*if(pFlashProducer_ != 0 && pFlashProducer_->pMonitor_) {
            std::wstring::size_type pos = str.find(TEXT('@'));
            if(pos != std::wstring::npos)
                pFlashProducer_->pMonitor_->Inform(str.substr(pos, str.find(TEXT('<'), pos)-pos));
        }*/
    } else if (str.find(L"OnNotify") != std::wstring::npos) {
        CASPAR_LOG(info) << print_() << L" [notification] " << str;

        // if(pFlashProducer_ != 0 && pFlashProducer_->pMonitor_) {
        //	std::wstring::size_type pos = str.find(TEXT('@'));
        //	if(pos != std::wstring::npos)
        //		pFlashProducer_->pMonitor_->Inform(str.substr(pos, str.find(TEXT('<'), pos)-pos));
        //}
    } else if (str.find(L"IsEmpty") != std::wstring::npos) {
        CASPAR_LOG(debug) << print_() << L" Empty.";
        ATLTRACE(_T("ShockwaveFlash::IsEmpty\n"));
        bIsEmpty_ = true;
    } else if (str.find(L"OnError") != std::wstring::npos) {
        if (str.find(L"No template playing on layer") != std::wstring::npos)
            CASPAR_LOG(info) << print_() << L" [info]        " << str;
        else
            CASPAR_LOG(error) << print_() << L" [error]        " << str;
    } else if (str.find(L"OnDebug") != std::wstring::npos) {
        CASPAR_LOG(debug) << print_() << L" [debug]        " << str;
    }
    // else if(str.find(TEXT("OnTemplateDescription")) != std::wstring::npos)
    //{
    //	CASPAR_LOG(error) << print_() << L" TemplateDescription: \n-------------------------------------------\n" << str
    //<<
    // L"\n-------------------------------------------";
    //}
    // else if(str.find(TEXT("OnGetInfo")) != std::wstring::npos)
    //{
    //	CASPAR_LOG(error) << print_() << L" Info: \n-------------------------------------------\n" << str <<
    // L"\n-------------------------------------------";
    //}
    // else
    //{
    //	CASPAR_LOG(error) << print_() << L" Unknown: \n-------------------------------------------\n" << str <<
    // L"\n-------------------------------------------";
    //}

    CComPtr<IShockwaveFlash> spFlash;
    HRESULT                  hr = m_spOleObject->QueryInterface(__uuidof(IShockwaveFlash), (void**)&spFlash);
    if (hr == S_OK && spFlash) {
        hr = spFlash->SetReturnValue(SysAllocString(L"<null/>"));
    }
}

void STDMETHODCALLTYPE FlashAxContainer::OnReadyStateChange(long newState)
{
    if (newState == 4)
        bReadyToRender_ = true;
}

void FlashAxContainer::DestroyAxControl()
{
    GetControllingUnknown()->AddRef();

    if (!m_spViewObject == false)
        m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, nullptr);

    if (!m_spOleObject == false) {
        DispEventUnadvise(m_spOleObject, &DIID__IShockwaveFlashEvents);
        m_spOleObject->Unadvise(m_dwOleObject);
        m_spOleObject->Close(OLECLOSE_NOSAVE);
        m_spOleObject->SetClientSite(nullptr);
    }

    if (!m_spUnknown == false) {
        CComPtr<IObjectWithSite> spSite;
        m_spUnknown->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
        if (spSite != nullptr)
            spSite->SetSite(nullptr);
    }

    if (!m_spViewObject == false)
        m_spViewObject.Release();

    if (!m_spInPlaceObjectWindowless == false)
        m_spInPlaceObjectWindowless.Release();

    if (!m_spOleObject == false)
        m_spOleObject.Release();

    if (!m_spUnknown == false)
        m_spUnknown.Release();
}

bool FlashAxContainer::CheckForFlashSupport()
{
    CLSID clsid;
    return SUCCEEDED(CLSIDFromString((LPOLESTR)flashGUID_, &clsid));
}

HRESULT
FlashAxContainer::CreateAxControl()
{
    CLSID   clsid;
    HRESULT hr = CLSIDFromString((LPOLESTR)flashGUID_, &clsid);
    if (SUCCEEDED(hr))
        hr = CoCreateInstance(clsid, nullptr, CLSCTX_INPROC_SERVER, __uuidof(IUnknown), (void**)&m_spUnknown);

    // Start ActivateAx
    if (SUCCEEDED(hr)) {
        m_spUnknown->QueryInterface(__uuidof(IOleObject), (void**)&m_spOleObject);
        if (m_spOleObject) {
            m_spOleObject->GetMiscStatus(DVASPECT_CONTENT, &m_dwMiscStatus);
            if (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST) {
                CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
                m_spOleObject->SetClientSite(spClientSite);
            }

            // Initialize control
            CComQIPtr<IPersistStreamInit> spPSI(m_spOleObject);
            if (spPSI)
                hr = spPSI->InitNew();

            if (FAILED(hr)) // If the initialization of the control failed...
            {
                // Clean up and return
                if (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)
                    m_spOleObject->SetClientSite(nullptr);

                m_dwMiscStatus = 0;
                m_spOleObject.Release();
                m_spUnknown.Release();

                return hr;
            }
            // end Initialize object

            if (0 == (m_dwMiscStatus & OLEMISC_SETCLIENTSITEFIRST)) {
                CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
                m_spOleObject->SetClientSite(spClientSite);
            }

            CComPtr<IShockwaveFlash> spFlash;
            HRESULT                  hr2 = m_spOleObject->QueryInterface(__uuidof(IShockwaveFlash), (void**)&spFlash);
            if (hr2 == S_OK && spFlash) {
                if (FAILED(spFlash->put_WMode(SysAllocString(L"Transparent"))))
                    CASPAR_LOG(warning) << print_() << L" Failed to set flash container to transparent mode.";
                // spFlash->put_WMode(TEXT("ogl"));
                HRESULT hResultQuality = spFlash->put_Quality2(SysAllocString(L"Best"));
            }
            if (SUCCEEDED(DispEventAdvise(spFlash, &DIID__IShockwaveFlashEvents))) {
            }

            HRESULT hrView = m_spOleObject->QueryInterface(__uuidof(IViewObjectEx), (void**)&m_spViewObject);

            CComQIPtr<IAdviseSink> spAdviseSink(GetControllingUnknown());
            m_spOleObject->Advise(spAdviseSink, &m_dwOleObject);
            if (m_spViewObject)
                m_spViewObject->SetAdvise(DVASPECT_CONTENT, 0, spAdviseSink);

            if ((m_dwMiscStatus & OLEMISC_INVISIBLEATRUNTIME) == 0) {
                // Initialize window to some dummy size
                m_rcPos.top    = 0;
                m_rcPos.left   = 0;
                m_rcPos.right  = 720;
                m_rcPos.bottom = 576;

                m_pxSize.cx = m_rcPos.right - m_rcPos.left;
                m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
                AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
                m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
                m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
                AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
                m_rcPos.right  = m_rcPos.left + m_pxSize.cx;
                m_rcPos.bottom = m_rcPos.top + m_pxSize.cy;

                CComQIPtr<IOleClientSite> spClientSite(GetControllingUnknown());
                hr = m_spOleObject->DoVerb(OLEIVERB_INPLACEACTIVATE, nullptr, spClientSite, 0, nullptr, &m_rcPos);
            }
        }
        CComPtr<IObjectWithSite> spSite;
        m_spUnknown->QueryInterface(__uuidof(IObjectWithSite), (void**)&spSite);
        if (spSite != nullptr)
            spSite->SetSite(GetControllingUnknown());
    }
    // End ActivateAx

    //	hr = E_FAIL;
    if (FAILED(hr) || m_spUnknown == nullptr) {
        return E_FAIL;
        // We don't have a control or something failed so release
        //		ReleaseAll();
    }

    return S_OK;
}

void FlashAxContainer::SetSize(size_t width, size_t height)
{
    if (m_spInPlaceObjectWindowless != nullptr) {
        m_rcPos.top    = 0;
        m_rcPos.left   = 0;
        m_rcPos.right  = width;
        m_rcPos.bottom = height;

        m_pxSize.cx = m_rcPos.right - m_rcPos.left;
        m_pxSize.cy = m_rcPos.bottom - m_rcPos.top;
        AtlPixelToHiMetric(&m_pxSize, &m_hmSize);
        m_spOleObject->SetExtent(DVASPECT_CONTENT, &m_hmSize);
        m_spOleObject->GetExtent(DVASPECT_CONTENT, &m_hmSize);
        AtlHiMetricToPixel(&m_hmSize, &m_pxSize);
        m_rcPos.right  = m_rcPos.left + m_pxSize.cx;
        m_rcPos.bottom = m_rcPos.top + m_pxSize.cy;

        m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
        bInvalidRect_ = true;
    }
}

HRESULT
FlashAxContainer::QueryControl(REFIID iid, void** ppUnk)
{
    ATLASSERT(ppUnk != NULL);
    if (ppUnk == nullptr)
        return E_POINTER;
    HRESULT hr = m_spOleObject->QueryInterface(iid, ppUnk);
    return hr;
}

bool FlashAxContainer::DrawControl(HDC targetDC)
{
    //	ATLTRACE(_T("FlashAxContainer::DrawControl\n"));
    DVASPECTINFO aspectInfo = {sizeof(DVASPECTINFO), DVASPECTINFOFLAG_CANOPTIMIZE};
    HRESULT      hr         = m_spViewObject->Draw(
        DVASPECT_CONTENT, -1, &aspectInfo, nullptr, nullptr, targetDC, nullptr, nullptr, nullptr, NULL);
    bInvalidRect_ = false;
    /*	const video_format_desc& fmtDesc = video_format_desc::FormatDescriptions[format_];

        //Trying to redraw just the dirty rectangles. Doesn't seem to work when the movie uses "filters", such as glow,
       dropshadow etc. std::vector<flash::DirtyRect>::iterator it = bDirtyRects_.begin();
        std::vector<flash::DirtyRect>::iterator end = bDirtyRects_.end();
        for(; it != end; ++it) {
            flash::DirtyRect& dirtyRect = (*it);
            if(dirtyRect.bWhole || dirtyRect.rect.right >= fmtDesc.width || dirtyRect.rect.bottom >= fmtDesc.height) {
                m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &m_rcPos);
                hr = m_spViewObject->Draw(DVASPECT_OPAQUE, -1, NULL, NULL, NULL, targetDC, NULL, NULL, NULL, NULL);
                break;
            }
            else {
                m_spInPlaceObjectWindowless->SetObjectRects(&m_rcPos, &(dirtyRect.rect));
                hr = m_spViewObject->Draw(DVASPECT_OPAQUE, -1, NULL, NULL, NULL, targetDC, NULL, NULL, NULL, NULL);
            }
        }
        bDirtyRects_.clear();
    */

    return hr == S_OK;
}

void FlashAxContainer::Tick()
{
    if (pTimerHelper) {
        DWORD time = pTimerHelper->Invoke(); // Tick flash
        if (time - timerCount_ >= 400) {
            timerCount_ = time;
            LRESULT hr;
            m_spInPlaceObjectWindowless->OnWindowMessage(WM_TIMER, 3, 0, &hr);
        }
    }
}

bool FlashAxContainer::FlashCall(const std::wstring& str, std::wstring& result2)
{
    CComBSTR                 result;
    CComPtr<IShockwaveFlash> spFlash;
    QueryControl(&spFlash);
    CComBSTR request(str.c_str());

    bIsEmpty_        = false;
    bCallSuccessful_ = false;
    for (size_t retries = 0; !bCallSuccessful_ && retries < 4; ++retries)
        spFlash->CallFunction(request, &result);

    if (bCallSuccessful_)
        result2 = result;

    return bCallSuccessful_;
}

}} // namespace caspar::flash
