

/* this ALWAYS GENERATED file contains the definitions for the interfaces */

/* File created by MIDL compiler version 6.00.0366 */
/* at Tue Mar 18 13:05:00 2008
 */
/* Compiler settings for .\flash\Flash9e.IDL:
    Oicf, W4, Zp8, env=Win32 (32b run)
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data
    VC __declspec() decoration level:
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
//@@MIDL_FILE_HEADING(  )

#pragma warning(disable : 4049) /* more than 64k source lines */

/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"
#include <dispex.h>

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef __axflash_h__
#define __axflash_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */

#ifndef __IShockwaveFlash_FWD_DEFINED__
#define __IShockwaveFlash_FWD_DEFINED__
typedef interface IShockwaveFlash IShockwaveFlash;
#endif /* __IShockwaveFlash_FWD_DEFINED__ */

#ifndef ___IShockwaveFlashEvents_FWD_DEFINED__
#define ___IShockwaveFlashEvents_FWD_DEFINED__
typedef interface _IShockwaveFlashEvents _IShockwaveFlashEvents;
#endif /* ___IShockwaveFlashEvents_FWD_DEFINED__ */

#ifndef __IFlashFactory_FWD_DEFINED__
#define __IFlashFactory_FWD_DEFINED__
typedef interface IFlashFactory IFlashFactory;
#endif /* __IFlashFactory_FWD_DEFINED__ */

#ifndef __IDispatchEx_FWD_DEFINED__
#define __IDispatchEx_FWD_DEFINED__
typedef interface IDispatchEx IDispatchEx;
#endif /* __IDispatchEx_FWD_DEFINED__ */

#ifndef __IFlashObjectInterface_FWD_DEFINED__
#define __IFlashObjectInterface_FWD_DEFINED__
typedef interface IFlashObjectInterface IFlashObjectInterface;
#endif /* __IFlashObjectInterface_FWD_DEFINED__ */

#ifndef __IServiceProvider_FWD_DEFINED__
#define __IServiceProvider_FWD_DEFINED__
typedef interface IServiceProvider IServiceProvider;
#endif /* __IServiceProvider_FWD_DEFINED__ */

#ifndef __ShockwaveFlash_FWD_DEFINED__
#define __ShockwaveFlash_FWD_DEFINED__

#ifdef __cplusplus
typedef class ShockwaveFlash ShockwaveFlash;
#else
typedef struct ShockwaveFlash       ShockwaveFlash;
#endif /* __cplusplus */

#endif /* __ShockwaveFlash_FWD_DEFINED__ */

#ifndef __FlashObjectInterface_FWD_DEFINED__
#define __FlashObjectInterface_FWD_DEFINED__

#ifdef __cplusplus
typedef class FlashObjectInterface FlashObjectInterface;
#else
typedef struct FlashObjectInterface FlashObjectInterface;
#endif /* __cplusplus */

#endif /* __FlashObjectInterface_FWD_DEFINED__ */

#ifdef __cplusplus
extern "C"
{
#endif

    void* __RPC_USER MIDL_user_allocate(size_t);
    void __RPC_USER MIDL_user_free(void*);

#ifndef __ShockwaveFlashObjects_LIBRARY_DEFINED__
#define __ShockwaveFlashObjects_LIBRARY_DEFINED__

    /* library ShockwaveFlashObjects */
    /* [custom][custom][helpstring][version][uuid] */

    EXTERN_C const IID LIBID_ShockwaveFlashObjects;

#ifndef __IShockwaveFlash_INTERFACE_DEFINED__
#define __IShockwaveFlash_INTERFACE_DEFINED__

    /* interface IShockwaveFlash */
    /* [object][oleautomation][dual][helpstring][uuid] */

    EXTERN_C const IID IID_IShockwaveFlash;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("D27CDB6C-AE6D-11CF-96B8-444553540000")
    IShockwaveFlash : public IDispatch
    {
      public:
        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ReadyState(
            /* [retval][out] */ long* pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_TotalFrames(
            /* [retval][out] */ long* pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Playing(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Playing(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Quality(
            /* [retval][out] */ int* pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Quality(
            /* [in] */ int pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ScaleMode(
            /* [retval][out] */ int* pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ScaleMode(
            /* [in] */ int pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AlignMode(
            /* [retval][out] */ int* pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AlignMode(
            /* [in] */ int pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BackgroundColor(
            /* [retval][out] */ long* pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_BackgroundColor(
            /* [in] */ long pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Loop(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Loop(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Movie(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Movie(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE receiveNum(
            /* [retval][out] */ long* pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FrameNum(
            /* [in] */ long pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetZoomRect(
            /* [in] */ long left,
            /* [in] */ long top,
            /* [in] */ long right,
            /* [in] */ long bottom) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Zoom(
            /* [in] */ int factor) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Pan(
            /* [in] */ long x,
            /* [in] */ long y,
            /* [in] */ int  mode) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Play(void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Stop(void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Back(void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Forward(void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE Rewind(void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE StopPlay(void) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GotoFrame(
            /* [in] */ long FrameNum) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CurrentFrame(
            /* [retval][out] */ long* FrameNum) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IsPlaying(
            /* [retval][out] */ VARIANT_BOOL * Playing) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE PercentLoaded(
            /* [retval][out] */ long* percent) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FrameLoaded(
            /* [in] */ long                   FrameNum,
            /* [retval][out] */ VARIANT_BOOL* loaded) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE FlashVersion(
            /* [retval][out] */ long* version) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_WMode(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_WMode(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SAlign(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_SAlign(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Menu(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Menu(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Base(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Base(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Scale(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Scale(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_DeviceFont(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_DeviceFont(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_EmbedMovie(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_EmbedMovie(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_BGColor(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_BGColor(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Quality2(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Quality2(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE LoadMovie(
            /* [in] */ int  layer,
            /* [in] */ BSTR url) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TGotoFrame(
            /* [in] */ BSTR target,
            /* [in] */ long FrameNum) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TGotoLabel(
            /* [in] */ BSTR target,
            /* [in] */ BSTR label) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TCurrentFrame(
            /* [in] */ BSTR           target,
            /* [retval][out] */ long* FrameNum) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TCurrentLabel(
            /* [in] */ BSTR target,
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TPlay(
            /* [in] */ BSTR target) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TStopPlay(
            /* [in] */ BSTR target) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetVariable(
            /* [in] */ BSTR name,
            /* [in] */ BSTR value) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE GetVariable(
            /* [in] */ BSTR name,
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TSetProperty(
            /* [in] */ BSTR target,
            /* [in] */ int  property,
            /* [in] */ BSTR value) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TGetProperty(
            /* [in] */ BSTR           target,
            /* [in] */ int            property,
            /* [retval][out] */ BSTR* pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TCallFrame(
            /* [in] */ BSTR target,
            /* [in] */ int  FrameNum) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TCallLabel(
            /* [in] */ BSTR target,
            /* [in] */ BSTR label) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TSetPropertyNum(
            /* [in] */ BSTR   target,
            /* [in] */ int    property,
            /* [in] */ double value) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TGetPropertyNum(
            /* [in] */ BSTR             target,
            /* [in] */ int              property,
            /* [retval][out] */ double* pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE TGetPropertyAsNumber(
            /* [in] */ BSTR             target,
            /* [in] */ int              property,
            /* [retval][out] */ double* pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SWRemote(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_SWRemote(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_FlashVars(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_FlashVars(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllowScriptAccess(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AllowScriptAccess(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_MovieData(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_MovieData(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_InlineData(
            /* [retval][out] */ IUnknown * *ppIUnknown) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_InlineData(
            /* [in] */ IUnknown * ppIUnknown) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_SeamlessTabbing(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_SeamlessTabbing(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE EnforceLocalSecurity(void) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_Profile(
            /* [retval][out] */ VARIANT_BOOL * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_Profile(
            /* [in] */ VARIANT_BOOL pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ProfileAddress(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ProfileAddress(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_ProfilePort(
            /* [retval][out] */ long* pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_ProfilePort(
            /* [in] */ long pVal) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE CallFunction(
            /* [in] */ BSTR request,
            /* [retval][out] */ BSTR * response) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE SetReturnValue(
            /* [in] */ BSTR returnValue) = 0;

        virtual /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE DisableLocalSecurity(void) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllowNetworking(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AllowNetworking(
            /* [in] */ BSTR pVal) = 0;

        virtual /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE get_AllowFullScreen(
            /* [retval][out] */ BSTR * pVal) = 0;

        virtual /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE put_AllowFullScreen(
            /* [in] */ BSTR pVal) = 0;
    };

#else /* C style interface */

    typedef struct IShockwaveFlashVtbl
    {
        BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)
        (IShockwaveFlash*           This,
         /* [in] */ REFIID          riid,
         /* [iid_is][out] */ void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(IShockwaveFlash* This);

        ULONG(STDMETHODCALLTYPE* Release)(IShockwaveFlash* This);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfoCount)
        (IShockwaveFlash*  This,
         /* [out] */ UINT* pctinfo);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfo)
        (IShockwaveFlash*        This,
         /* [in] */ UINT         iTInfo,
         /* [in] */ LCID         lcid,
         /* [out] */ ITypeInfo** ppTInfo);

        HRESULT(STDMETHODCALLTYPE* GetIDsOfNames)
        (IShockwaveFlash*              This,
         /* [in] */ REFIID             riid,
         /* [size_is][in] */ LPOLESTR* rgszNames,
         /* [in] */ UINT               cNames,
         /* [in] */ LCID               lcid,
         /* [size_is][out] */ DISPID*  rgDispId);

        /* [local] */ HRESULT(STDMETHODCALLTYPE* Invoke)(IShockwaveFlash*            This,
                                                         /* [in] */ DISPID           dispIdMember,
                                                         /* [in] */ REFIID           riid,
                                                         /* [in] */ LCID             lcid,
                                                         /* [in] */ WORD             wFlags,
                                                         /* [out][in] */ DISPPARAMS* pDispParams,
                                                         /* [out] */ VARIANT*        pVarResult,
                                                         /* [out] */ EXCEPINFO*      pExcepInfo,
                                                         /* [out] */ UINT*           puArgErr);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_ReadyState)(IShockwaveFlash*          This,
                                                                                   /* [retval][out] */ long* pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_TotalFrames)(IShockwaveFlash*          This,
                                                                                    /* [retval][out] */ long* pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Playing)(IShockwaveFlash*                  This,
                                                                                /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Playing)(IShockwaveFlash*        This,
                                                                                /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Quality)(IShockwaveFlash*         This,
                                                                                /* [retval][out] */ int* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Quality)(IShockwaveFlash* This,
                                                                                /* [in] */ int   pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_ScaleMode)(IShockwaveFlash*         This,
                                                                                  /* [retval][out] */ int* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_ScaleMode)(IShockwaveFlash* This,
                                                                                  /* [in] */ int   pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_AlignMode)(IShockwaveFlash*         This,
                                                                                  /* [retval][out] */ int* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_AlignMode)(IShockwaveFlash* This,
                                                                                  /* [in] */ int   pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_BackgroundColor)(IShockwaveFlash*          This,
                                                                                        /* [retval][out] */ long* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_BackgroundColor)(IShockwaveFlash* This,
                                                                                        /* [in] */ long  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Loop)(IShockwaveFlash*                  This,
                                                                             /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Loop)(IShockwaveFlash*        This,
                                                                             /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Movie)(IShockwaveFlash*          This,
                                                                              /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Movie)(IShockwaveFlash* This,
                                                                              /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* receiveNum)(IShockwaveFlash*          This,
                                                                               /* [retval][out] */ long* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_FrameNum)(IShockwaveFlash* This,
                                                                                 /* [in] */ long  pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* SetZoomRect)(IShockwaveFlash* This,
                                                                       /* [in] */ long  left,
                                                                       /* [in] */ long  top,
                                                                       /* [in] */ long  right,
                                                                       /* [in] */ long  bottom);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Zoom)(IShockwaveFlash* This,
                                                                /* [in] */ int   factor);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Pan)(IShockwaveFlash* This,
                                                               /* [in] */ long  x,
                                                               /* [in] */ long  y,
                                                               /* [in] */ int   mode);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Play)(IShockwaveFlash* This);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Stop)(IShockwaveFlash* This);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Back)(IShockwaveFlash* This);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Forward)(IShockwaveFlash* This);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* Rewind)(IShockwaveFlash* This);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* StopPlay)(IShockwaveFlash* This);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* GotoFrame)(IShockwaveFlash* This,
                                                                     /* [in] */ long  FrameNum);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* CurrentFrame)(IShockwaveFlash*          This,
                                                                        /* [retval][out] */ long* FrameNum);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* IsPlaying)(IShockwaveFlash*                  This,
                                                                     /* [retval][out] */ VARIANT_BOOL* Playing);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* PercentLoaded)(IShockwaveFlash*          This,
                                                                         /* [retval][out] */ long* percent);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* FrameLoaded)(IShockwaveFlash*                  This,
                                                                       /* [in] */ long                   FrameNum,
                                                                       /* [retval][out] */ VARIANT_BOOL* loaded);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* FlashVersion)(IShockwaveFlash*          This,
                                                                        /* [retval][out] */ long* version);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_WMode)(IShockwaveFlash*          This,
                                                                              /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_WMode)(IShockwaveFlash* This,
                                                                              /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_SAlign)(IShockwaveFlash*          This,
                                                                               /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_SAlign)(IShockwaveFlash* This,
                                                                               /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Menu)(IShockwaveFlash*                  This,
                                                                             /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Menu)(IShockwaveFlash*        This,
                                                                             /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Base)(IShockwaveFlash*          This,
                                                                             /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Base)(IShockwaveFlash* This,
                                                                             /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Scale)(IShockwaveFlash*          This,
                                                                              /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Scale)(IShockwaveFlash* This,
                                                                              /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_DeviceFont)(
            IShockwaveFlash*                  This,
            /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_DeviceFont)(IShockwaveFlash*        This,
                                                                                   /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_EmbedMovie)(
            IShockwaveFlash*                  This,
            /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_EmbedMovie)(IShockwaveFlash*        This,
                                                                                   /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_BGColor)(IShockwaveFlash*          This,
                                                                                /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_BGColor)(IShockwaveFlash* This,
                                                                                /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Quality2)(IShockwaveFlash*          This,
                                                                                 /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Quality2)(IShockwaveFlash* This,
                                                                                 /* [in] */ BSTR  pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* LoadMovie)(IShockwaveFlash* This,
                                                                     /* [in] */ int   layer,
                                                                     /* [in] */ BSTR  url);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TGotoFrame)(IShockwaveFlash* This,
                                                                      /* [in] */ BSTR  target,
                                                                      /* [in] */ long  FrameNum);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TGotoLabel)(IShockwaveFlash* This,
                                                                      /* [in] */ BSTR  target,
                                                                      /* [in] */ BSTR  label);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TCurrentFrame)(IShockwaveFlash*          This,
                                                                         /* [in] */ BSTR           target,
                                                                         /* [retval][out] */ long* FrameNum);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TCurrentLabel)(IShockwaveFlash*          This,
                                                                         /* [in] */ BSTR           target,
                                                                         /* [retval][out] */ BSTR* pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TPlay)(IShockwaveFlash* This,
                                                                 /* [in] */ BSTR  target);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TStopPlay)(IShockwaveFlash* This,
                                                                     /* [in] */ BSTR  target);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* SetVariable)(IShockwaveFlash* This,
                                                                       /* [in] */ BSTR  name,
                                                                       /* [in] */ BSTR  value);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* GetVariable)(IShockwaveFlash*          This,
                                                                       /* [in] */ BSTR           name,
                                                                       /* [retval][out] */ BSTR* pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TSetProperty)(IShockwaveFlash* This,
                                                                        /* [in] */ BSTR  target,
                                                                        /* [in] */ int   property,
                                                                        /* [in] */ BSTR  value);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TGetProperty)(IShockwaveFlash*          This,
                                                                        /* [in] */ BSTR           target,
                                                                        /* [in] */ int            property,
                                                                        /* [retval][out] */ BSTR* pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TCallFrame)(IShockwaveFlash* This,
                                                                      /* [in] */ BSTR  target,
                                                                      /* [in] */ int   FrameNum);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TCallLabel)(IShockwaveFlash* This,
                                                                      /* [in] */ BSTR  target,
                                                                      /* [in] */ BSTR  label);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TSetPropertyNum)(IShockwaveFlash*  This,
                                                                           /* [in] */ BSTR   target,
                                                                           /* [in] */ int    property,
                                                                           /* [in] */ double value);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TGetPropertyNum)(IShockwaveFlash*            This,
                                                                           /* [in] */ BSTR             target,
                                                                           /* [in] */ int              property,
                                                                           /* [retval][out] */ double* pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* TGetPropertyAsNumber)(IShockwaveFlash*            This,
                                                                                /* [in] */ BSTR             target,
                                                                                /* [in] */ int              property,
                                                                                /* [retval][out] */ double* pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_SWRemote)(IShockwaveFlash*          This,
                                                                                 /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_SWRemote)(IShockwaveFlash* This,
                                                                                 /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_FlashVars)(IShockwaveFlash*          This,
                                                                                  /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_FlashVars)(IShockwaveFlash* This,
                                                                                  /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_AllowScriptAccess)(
            IShockwaveFlash*          This,
            /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_AllowScriptAccess)(IShockwaveFlash* This,
                                                                                          /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_MovieData)(IShockwaveFlash*          This,
                                                                                  /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_MovieData)(IShockwaveFlash* This,
                                                                                  /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_InlineData)(
            IShockwaveFlash*               This,
            /* [retval][out] */ IUnknown** ppIUnknown);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_InlineData)(IShockwaveFlash*     This,
                                                                                   /* [in] */ IUnknown* ppIUnknown);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_SeamlessTabbing)(
            IShockwaveFlash*                  This,
            /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_SeamlessTabbing)(IShockwaveFlash*        This,
                                                                                        /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* EnforceLocalSecurity)(IShockwaveFlash* This);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_Profile)(IShockwaveFlash*                  This,
                                                                                /* [retval][out] */ VARIANT_BOOL* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_Profile)(IShockwaveFlash*        This,
                                                                                /* [in] */ VARIANT_BOOL pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_ProfileAddress)(IShockwaveFlash*          This,
                                                                                       /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_ProfileAddress)(IShockwaveFlash* This,
                                                                                       /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_ProfilePort)(IShockwaveFlash*          This,
                                                                                    /* [retval][out] */ long* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_ProfilePort)(IShockwaveFlash* This,
                                                                                    /* [in] */ long  pVal);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* CallFunction)(IShockwaveFlash*          This,
                                                                        /* [in] */ BSTR           request,
                                                                        /* [retval][out] */ BSTR* response);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* SetReturnValue)(IShockwaveFlash* This,
                                                                          /* [in] */ BSTR  returnValue);

        /* [helpstring][id] */ HRESULT(STDMETHODCALLTYPE* DisableLocalSecurity)(IShockwaveFlash* This);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_AllowNetworking)(IShockwaveFlash*          This,
                                                                                        /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_AllowNetworking)(IShockwaveFlash* This,
                                                                                        /* [in] */ BSTR  pVal);

        /* [helpstring][propget][id] */ HRESULT(STDMETHODCALLTYPE* get_AllowFullScreen)(IShockwaveFlash*          This,
                                                                                        /* [retval][out] */ BSTR* pVal);

        /* [helpstring][propput][id] */ HRESULT(STDMETHODCALLTYPE* put_AllowFullScreen)(IShockwaveFlash* This,
                                                                                        /* [in] */ BSTR  pVal);

        END_INTERFACE
    } IShockwaveFlashVtbl;

    interface IShockwaveFlash { CONST_VTBL struct IShockwaveFlashVtbl* lpVtbl; };

#ifdef COBJMACROS

#define IShockwaveFlash_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IShockwaveFlash_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IShockwaveFlash_Release(This) (This)->lpVtbl->Release(This)

#define IShockwaveFlash_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IShockwaveFlash_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                                       \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IShockwaveFlash_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                                   \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IShockwaveFlash_Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)  \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)

#define IShockwaveFlash_get_ReadyState(This, pVal) (This)->lpVtbl->get_ReadyState(This, pVal)

#define IShockwaveFlash_get_TotalFrames(This, pVal) (This)->lpVtbl->get_TotalFrames(This, pVal)

#define IShockwaveFlash_get_Playing(This, pVal) (This)->lpVtbl->get_Playing(This, pVal)

#define IShockwaveFlash_put_Playing(This, pVal) (This)->lpVtbl->put_Playing(This, pVal)

#define IShockwaveFlash_get_Quality(This, pVal) (This)->lpVtbl->get_Quality(This, pVal)

#define IShockwaveFlash_put_Quality(This, pVal) (This)->lpVtbl->put_Quality(This, pVal)

#define IShockwaveFlash_get_ScaleMode(This, pVal) (This)->lpVtbl->get_ScaleMode(This, pVal)

#define IShockwaveFlash_put_ScaleMode(This, pVal) (This)->lpVtbl->put_ScaleMode(This, pVal)

#define IShockwaveFlash_get_AlignMode(This, pVal) (This)->lpVtbl->get_AlignMode(This, pVal)

#define IShockwaveFlash_put_AlignMode(This, pVal) (This)->lpVtbl->put_AlignMode(This, pVal)

#define IShockwaveFlash_get_BackgroundColor(This, pVal) (This)->lpVtbl->get_BackgroundColor(This, pVal)

#define IShockwaveFlash_put_BackgroundColor(This, pVal) (This)->lpVtbl->put_BackgroundColor(This, pVal)

#define IShockwaveFlash_get_Loop(This, pVal) (This)->lpVtbl->get_Loop(This, pVal)

#define IShockwaveFlash_put_Loop(This, pVal) (This)->lpVtbl->put_Loop(This, pVal)

#define IShockwaveFlash_get_Movie(This, pVal) (This)->lpVtbl->get_Movie(This, pVal)

#define IShockwaveFlash_put_Movie(This, pVal) (This)->lpVtbl->put_Movie(This, pVal)

#define IShockwaveFlash_receiveNum(This, pVal) (This)->lpVtbl->receiveNum(This, pVal)

#define IShockwaveFlash_put_FrameNum(This, pVal) (This)->lpVtbl->put_FrameNum(This, pVal)

#define IShockwaveFlash_SetZoomRect(This, left, top, right, bottom)                                                    \
    (This)->lpVtbl->SetZoomRect(This, left, top, right, bottom)

#define IShockwaveFlash_Zoom(This, factor) (This)->lpVtbl->Zoom(This, factor)

#define IShockwaveFlash_Pan(This, x, y, mode) (This)->lpVtbl->Pan(This, x, y, mode)

#define IShockwaveFlash_Play(This) (This)->lpVtbl->Play(This)

#define IShockwaveFlash_Stop(This) (This)->lpVtbl->Stop(This)

#define IShockwaveFlash_Back(This) (This)->lpVtbl->Back(This)

#define IShockwaveFlash_Forward(This) (This)->lpVtbl->Forward(This)

#define IShockwaveFlash_Rewind(This) (This)->lpVtbl->Rewind(This)

#define IShockwaveFlash_StopPlay(This) (This)->lpVtbl->StopPlay(This)

#define IShockwaveFlash_GotoFrame(This, FrameNum) (This)->lpVtbl->GotoFrame(This, FrameNum)

#define IShockwaveFlash_CurrentFrame(This, FrameNum) (This)->lpVtbl->CurrentFrame(This, FrameNum)

#define IShockwaveFlash_IsPlaying(This, Playing) (This)->lpVtbl->IsPlaying(This, Playing)

#define IShockwaveFlash_PercentLoaded(This, percent) (This)->lpVtbl->PercentLoaded(This, percent)

#define IShockwaveFlash_FrameLoaded(This, FrameNum, loaded) (This)->lpVtbl->FrameLoaded(This, FrameNum, loaded)

#define IShockwaveFlash_FlashVersion(This, version) (This)->lpVtbl->FlashVersion(This, version)

#define IShockwaveFlash_get_WMode(This, pVal) (This)->lpVtbl->get_WMode(This, pVal)

#define IShockwaveFlash_put_WMode(This, pVal) (This)->lpVtbl->put_WMode(This, pVal)

#define IShockwaveFlash_get_SAlign(This, pVal) (This)->lpVtbl->get_SAlign(This, pVal)

#define IShockwaveFlash_put_SAlign(This, pVal) (This)->lpVtbl->put_SAlign(This, pVal)

#define IShockwaveFlash_get_Menu(This, pVal) (This)->lpVtbl->get_Menu(This, pVal)

#define IShockwaveFlash_put_Menu(This, pVal) (This)->lpVtbl->put_Menu(This, pVal)

#define IShockwaveFlash_get_Base(This, pVal) (This)->lpVtbl->get_Base(This, pVal)

#define IShockwaveFlash_put_Base(This, pVal) (This)->lpVtbl->put_Base(This, pVal)

#define IShockwaveFlash_get_Scale(This, pVal) (This)->lpVtbl->get_Scale(This, pVal)

#define IShockwaveFlash_put_Scale(This, pVal) (This)->lpVtbl->put_Scale(This, pVal)

#define IShockwaveFlash_get_DeviceFont(This, pVal) (This)->lpVtbl->get_DeviceFont(This, pVal)

#define IShockwaveFlash_put_DeviceFont(This, pVal) (This)->lpVtbl->put_DeviceFont(This, pVal)

#define IShockwaveFlash_get_EmbedMovie(This, pVal) (This)->lpVtbl->get_EmbedMovie(This, pVal)

#define IShockwaveFlash_put_EmbedMovie(This, pVal) (This)->lpVtbl->put_EmbedMovie(This, pVal)

#define IShockwaveFlash_get_BGColor(This, pVal) (This)->lpVtbl->get_BGColor(This, pVal)

#define IShockwaveFlash_put_BGColor(This, pVal) (This)->lpVtbl->put_BGColor(This, pVal)

#define IShockwaveFlash_get_Quality2(This, pVal) (This)->lpVtbl->get_Quality2(This, pVal)

#define IShockwaveFlash_put_Quality2(This, pVal) (This)->lpVtbl->put_Quality2(This, pVal)

#define IShockwaveFlash_LoadMovie(This, layer, url) (This)->lpVtbl->LoadMovie(This, layer, url)

#define IShockwaveFlash_TGotoFrame(This, target, FrameNum) (This)->lpVtbl->TGotoFrame(This, target, FrameNum)

#define IShockwaveFlash_TGotoLabel(This, target, label) (This)->lpVtbl->TGotoLabel(This, target, label)

#define IShockwaveFlash_TCurrentFrame(This, target, FrameNum) (This)->lpVtbl->TCurrentFrame(This, target, FrameNum)

#define IShockwaveFlash_TCurrentLabel(This, target, pVal) (This)->lpVtbl->TCurrentLabel(This, target, pVal)

#define IShockwaveFlash_TPlay(This, target) (This)->lpVtbl->TPlay(This, target)

#define IShockwaveFlash_TStopPlay(This, target) (This)->lpVtbl->TStopPlay(This, target)

#define IShockwaveFlash_SetVariable(This, name, value) (This)->lpVtbl->SetVariable(This, name, value)

#define IShockwaveFlash_GetVariable(This, name, pVal) (This)->lpVtbl->GetVariable(This, name, pVal)

#define IShockwaveFlash_TSetProperty(This, target, property, value)                                                    \
    (This)->lpVtbl->TSetProperty(This, target, property, value)

#define IShockwaveFlash_TGetProperty(This, target, property, pVal)                                                     \
    (This)->lpVtbl->TGetProperty(This, target, property, pVal)

#define IShockwaveFlash_TCallFrame(This, target, FrameNum) (This)->lpVtbl->TCallFrame(This, target, FrameNum)

#define IShockwaveFlash_TCallLabel(This, target, label) (This)->lpVtbl->TCallLabel(This, target, label)

#define IShockwaveFlash_TSetPropertyNum(This, target, property, value)                                                 \
    (This)->lpVtbl->TSetPropertyNum(This, target, property, value)

#define IShockwaveFlash_TGetPropertyNum(This, target, property, pVal)                                                  \
    (This)->lpVtbl->TGetPropertyNum(This, target, property, pVal)

#define IShockwaveFlash_TGetPropertyAsNumber(This, target, property, pVal)                                             \
    (This)->lpVtbl->TGetPropertyAsNumber(This, target, property, pVal)

#define IShockwaveFlash_get_SWRemote(This, pVal) (This)->lpVtbl->get_SWRemote(This, pVal)

#define IShockwaveFlash_put_SWRemote(This, pVal) (This)->lpVtbl->put_SWRemote(This, pVal)

#define IShockwaveFlash_get_FlashVars(This, pVal) (This)->lpVtbl->get_FlashVars(This, pVal)

#define IShockwaveFlash_put_FlashVars(This, pVal) (This)->lpVtbl->put_FlashVars(This, pVal)

#define IShockwaveFlash_get_AllowScriptAccess(This, pVal) (This)->lpVtbl->get_AllowScriptAccess(This, pVal)

#define IShockwaveFlash_put_AllowScriptAccess(This, pVal) (This)->lpVtbl->put_AllowScriptAccess(This, pVal)

#define IShockwaveFlash_get_MovieData(This, pVal) (This)->lpVtbl->get_MovieData(This, pVal)

#define IShockwaveFlash_put_MovieData(This, pVal) (This)->lpVtbl->put_MovieData(This, pVal)

#define IShockwaveFlash_get_InlineData(This, ppIUnknown) (This)->lpVtbl->get_InlineData(This, ppIUnknown)

#define IShockwaveFlash_put_InlineData(This, ppIUnknown) (This)->lpVtbl->put_InlineData(This, ppIUnknown)

#define IShockwaveFlash_get_SeamlessTabbing(This, pVal) (This)->lpVtbl->get_SeamlessTabbing(This, pVal)

#define IShockwaveFlash_put_SeamlessTabbing(This, pVal) (This)->lpVtbl->put_SeamlessTabbing(This, pVal)

#define IShockwaveFlash_EnforceLocalSecurity(This) (This)->lpVtbl->EnforceLocalSecurity(This)

#define IShockwaveFlash_get_Profile(This, pVal) (This)->lpVtbl->get_Profile(This, pVal)

#define IShockwaveFlash_put_Profile(This, pVal) (This)->lpVtbl->put_Profile(This, pVal)

#define IShockwaveFlash_get_ProfileAddress(This, pVal) (This)->lpVtbl->get_ProfileAddress(This, pVal)

#define IShockwaveFlash_put_ProfileAddress(This, pVal) (This)->lpVtbl->put_ProfileAddress(This, pVal)

#define IShockwaveFlash_get_ProfilePort(This, pVal) (This)->lpVtbl->get_ProfilePort(This, pVal)

#define IShockwaveFlash_put_ProfilePort(This, pVal) (This)->lpVtbl->put_ProfilePort(This, pVal)

#define IShockwaveFlash_CallFunction(This, request, response) (This)->lpVtbl->CallFunction(This, request, response)

#define IShockwaveFlash_SetReturnValue(This, returnValue) (This)->lpVtbl->SetReturnValue(This, returnValue)

#define IShockwaveFlash_DisableLocalSecurity(This) (This)->lpVtbl->DisableLocalSecurity(This)

#define IShockwaveFlash_get_AllowNetworking(This, pVal) (This)->lpVtbl->get_AllowNetworking(This, pVal)

#define IShockwaveFlash_put_AllowNetworking(This, pVal) (This)->lpVtbl->put_AllowNetworking(This, pVal)

#define IShockwaveFlash_get_AllowFullScreen(This, pVal) (This)->lpVtbl->get_AllowFullScreen(This, pVal)

#define IShockwaveFlash_put_AllowFullScreen(This, pVal) (This)->lpVtbl->put_AllowFullScreen(This, pVal)

#endif /* COBJMACROS */

#endif /* C style interface */

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_ReadyState_Proxy(IShockwaveFlash*          This,
                                                                                 /* [retval][out] */ long* pVal);

    void __RPC_STUB IShockwaveFlash_get_ReadyState_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_TotalFrames_Proxy(IShockwaveFlash*          This,
                                                                                  /* [retval][out] */ long* pVal);

    void __RPC_STUB IShockwaveFlash_get_TotalFrames_Stub(IRpcStubBuffer*    This,
                                                         IRpcChannelBuffer* _pRpcChannelBuffer,
                                                         PRPC_MESSAGE       _pRpcMessage,
                                                         DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Playing_Proxy(IShockwaveFlash*                  This,
                                                                              /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_Playing_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_Playing_Proxy(IShockwaveFlash*        This,
                                                                              /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_Playing_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Quality_Proxy(IShockwaveFlash*         This,
                                                                              /* [retval][out] */ int* pVal);

    void __RPC_STUB IShockwaveFlash_get_Quality_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_Quality_Proxy(IShockwaveFlash* This,
                                                                                                /* [in] */ int   pVal);

    void __RPC_STUB IShockwaveFlash_put_Quality_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_ScaleMode_Proxy(IShockwaveFlash*         This,
                                                                                /* [retval][out] */ int* pVal);

    void __RPC_STUB IShockwaveFlash_get_ScaleMode_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_ScaleMode_Proxy(IShockwaveFlash* This,
                                                                                                  /* [in] */ int pVal);

    void __RPC_STUB IShockwaveFlash_put_ScaleMode_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_AlignMode_Proxy(IShockwaveFlash*         This,
                                                                                /* [retval][out] */ int* pVal);

    void __RPC_STUB IShockwaveFlash_get_AlignMode_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_AlignMode_Proxy(IShockwaveFlash* This,
                                                                                                  /* [in] */ int pVal);

    void __RPC_STUB IShockwaveFlash_put_AlignMode_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_BackgroundColor_Proxy(IShockwaveFlash*          This,
                                                                                      /* [retval][out] */ long* pVal);

    void __RPC_STUB IShockwaveFlash_get_BackgroundColor_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_BackgroundColor_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ long  pVal);

    void __RPC_STUB IShockwaveFlash_put_BackgroundColor_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Loop_Proxy(IShockwaveFlash*                  This,
                                                                           /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_Loop_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_Loop_Proxy(IShockwaveFlash*        This,
                                                                           /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_Loop_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Movie_Proxy(IShockwaveFlash*          This,
                                                                            /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_Movie_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_Movie_Proxy(IShockwaveFlash* This,
                                                                                              /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_Movie_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_receiveNum_Proxy(IShockwaveFlash*          This,
                                                                             /* [retval][out] */ long* pVal);

    void __RPC_STUB IShockwaveFlash_receiveNum_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_FrameNum_Proxy(IShockwaveFlash* This,
                                                                                                 /* [in] */ long  pVal);

    void __RPC_STUB IShockwaveFlash_put_FrameNum_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_SetZoomRect_Proxy(IShockwaveFlash* This,
                                                                                       /* [in] */ long  left,
                                                                                       /* [in] */ long  top,
                                                                                       /* [in] */ long  right,
                                                                                       /* [in] */ long  bottom);

    void __RPC_STUB IShockwaveFlash_SetZoomRect_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Zoom_Proxy(IShockwaveFlash* This,
                                                                                /* [in] */ int   factor);

    void __RPC_STUB IShockwaveFlash_Zoom_Stub(IRpcStubBuffer*    This,
                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                              PRPC_MESSAGE       _pRpcMessage,
                                              DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Pan_Proxy(IShockwaveFlash* This,
                                                                               /* [in] */ long  x,
                                                                               /* [in] */ long  y,
                                                                               /* [in] */ int   mode);

    void __RPC_STUB IShockwaveFlash_Pan_Stub(IRpcStubBuffer*    This,
                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                             PRPC_MESSAGE       _pRpcMessage,
                                             DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Play_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_Play_Stub(IRpcStubBuffer*    This,
                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                              PRPC_MESSAGE       _pRpcMessage,
                                              DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Stop_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_Stop_Stub(IRpcStubBuffer*    This,
                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                              PRPC_MESSAGE       _pRpcMessage,
                                              DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Back_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_Back_Stub(IRpcStubBuffer*    This,
                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                              PRPC_MESSAGE       _pRpcMessage,
                                              DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Forward_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_Forward_Stub(IRpcStubBuffer*    This,
                                                 IRpcChannelBuffer* _pRpcChannelBuffer,
                                                 PRPC_MESSAGE       _pRpcMessage,
                                                 DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_Rewind_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_Rewind_Stub(IRpcStubBuffer*    This,
                                                IRpcChannelBuffer* _pRpcChannelBuffer,
                                                PRPC_MESSAGE       _pRpcMessage,
                                                DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_StopPlay_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_StopPlay_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_GotoFrame_Proxy(IShockwaveFlash* This,
                                                                                     /* [in] */ long  FrameNum);

    void __RPC_STUB IShockwaveFlash_GotoFrame_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_CurrentFrame_Proxy(IShockwaveFlash*          This,
                                                                      /* [retval][out] */ long* FrameNum);

    void __RPC_STUB IShockwaveFlash_CurrentFrame_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_IsPlaying_Proxy(IShockwaveFlash*                  This,
                                                                   /* [retval][out] */ VARIANT_BOOL* Playing);

    void __RPC_STUB IShockwaveFlash_IsPlaying_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_PercentLoaded_Proxy(IShockwaveFlash*          This,
                                                                       /* [retval][out] */ long* percent);

    void __RPC_STUB IShockwaveFlash_PercentLoaded_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_FrameLoaded_Proxy(IShockwaveFlash*                  This,
                                                                     /* [in] */ long                   FrameNum,
                                                                     /* [retval][out] */ VARIANT_BOOL* loaded);

    void __RPC_STUB IShockwaveFlash_FrameLoaded_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_FlashVersion_Proxy(IShockwaveFlash*          This,
                                                                      /* [retval][out] */ long* version);

    void __RPC_STUB IShockwaveFlash_FlashVersion_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_WMode_Proxy(IShockwaveFlash*          This,
                                                                            /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_WMode_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_WMode_Proxy(IShockwaveFlash* This,
                                                                                              /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_WMode_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_SAlign_Proxy(IShockwaveFlash*          This,
                                                                             /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_SAlign_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_SAlign_Proxy(IShockwaveFlash* This,
                                                                                               /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_SAlign_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Menu_Proxy(IShockwaveFlash*                  This,
                                                                           /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_Menu_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_Menu_Proxy(IShockwaveFlash*        This,
                                                                           /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_Menu_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Base_Proxy(IShockwaveFlash*          This,
                                                                           /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_Base_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_Base_Proxy(IShockwaveFlash* This,
                                                                                             /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_Base_Stub(IRpcStubBuffer*    This,
                                                  IRpcChannelBuffer* _pRpcChannelBuffer,
                                                  PRPC_MESSAGE       _pRpcMessage,
                                                  DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Scale_Proxy(IShockwaveFlash*          This,
                                                                            /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_Scale_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_Scale_Proxy(IShockwaveFlash* This,
                                                                                              /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_Scale_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_DeviceFont_Proxy(IShockwaveFlash*                  This,
                                                                                 /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_DeviceFont_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_DeviceFont_Proxy(IShockwaveFlash*        This,
                                                                                 /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_DeviceFont_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_EmbedMovie_Proxy(IShockwaveFlash*                  This,
                                                                                 /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_EmbedMovie_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_EmbedMovie_Proxy(IShockwaveFlash*        This,
                                                                                 /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_EmbedMovie_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_BGColor_Proxy(IShockwaveFlash*          This,
                                                                              /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_BGColor_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_BGColor_Proxy(IShockwaveFlash* This,
                                                                                                /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_BGColor_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Quality2_Proxy(IShockwaveFlash*          This,
                                                                               /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_Quality2_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_Quality2_Proxy(IShockwaveFlash* This,
                                                                                                 /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_Quality2_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_LoadMovie_Proxy(IShockwaveFlash* This,
                                                                                     /* [in] */ int   layer,
                                                                                     /* [in] */ BSTR  url);

    void __RPC_STUB IShockwaveFlash_LoadMovie_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TGotoFrame_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ BSTR  target,
                                                                                      /* [in] */ long  FrameNum);

    void __RPC_STUB IShockwaveFlash_TGotoFrame_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TGotoLabel_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ BSTR  target,
                                                                                      /* [in] */ BSTR  label);

    void __RPC_STUB IShockwaveFlash_TGotoLabel_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_TCurrentFrame_Proxy(IShockwaveFlash*          This,
                                                                       /* [in] */ BSTR           target,
                                                                       /* [retval][out] */ long* FrameNum);

    void __RPC_STUB IShockwaveFlash_TCurrentFrame_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_TCurrentLabel_Proxy(IShockwaveFlash*          This,
                                                                       /* [in] */ BSTR           target,
                                                                       /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_TCurrentLabel_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TPlay_Proxy(IShockwaveFlash* This,
                                                                                 /* [in] */ BSTR  target);

    void __RPC_STUB IShockwaveFlash_TPlay_Stub(IRpcStubBuffer*    This,
                                               IRpcChannelBuffer* _pRpcChannelBuffer,
                                               PRPC_MESSAGE       _pRpcMessage,
                                               DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TStopPlay_Proxy(IShockwaveFlash* This,
                                                                                     /* [in] */ BSTR  target);

    void __RPC_STUB IShockwaveFlash_TStopPlay_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_SetVariable_Proxy(IShockwaveFlash* This,
                                                                                       /* [in] */ BSTR  name,
                                                                                       /* [in] */ BSTR  value);

    void __RPC_STUB IShockwaveFlash_SetVariable_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_GetVariable_Proxy(IShockwaveFlash*          This,
                                                                                       /* [in] */ BSTR           name,
                                                                                       /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_GetVariable_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TSetProperty_Proxy(IShockwaveFlash* This,
                                                                                        /* [in] */ BSTR  target,
                                                                                        /* [in] */ int   property,
                                                                                        /* [in] */ BSTR  value);

    void __RPC_STUB IShockwaveFlash_TSetProperty_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TGetProperty_Proxy(IShockwaveFlash* This,
                                                                                        /* [in] */ BSTR  target,
                                                                                        /* [in] */ int   property,
                                                                                        /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_TGetProperty_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TCallFrame_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ BSTR  target,
                                                                                      /* [in] */ int   FrameNum);

    void __RPC_STUB IShockwaveFlash_TCallFrame_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TCallLabel_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ BSTR  target,
                                                                                      /* [in] */ BSTR  label);

    void __RPC_STUB IShockwaveFlash_TCallLabel_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_TSetPropertyNum_Proxy(IShockwaveFlash*  This,
                                                                                           /* [in] */ BSTR   target,
                                                                                           /* [in] */ int    property,
                                                                                           /* [in] */ double value);

    void __RPC_STUB IShockwaveFlash_TSetPropertyNum_Stub(IRpcStubBuffer*    This,
                                                         IRpcChannelBuffer* _pRpcChannelBuffer,
                                                         PRPC_MESSAGE       _pRpcMessage,
                                                         DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_TGetPropertyNum_Proxy(IShockwaveFlash*            This,
                                                                         /* [in] */ BSTR             target,
                                                                         /* [in] */ int              property,
                                                                         /* [retval][out] */ double* pVal);

    void __RPC_STUB IShockwaveFlash_TGetPropertyNum_Stub(IRpcStubBuffer*    This,
                                                         IRpcChannelBuffer* _pRpcChannelBuffer,
                                                         PRPC_MESSAGE       _pRpcMessage,
                                                         DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_TGetPropertyAsNumber_Proxy(IShockwaveFlash*            This,
                                                                              /* [in] */ BSTR             target,
                                                                              /* [in] */ int              property,
                                                                              /* [retval][out] */ double* pVal);

    void __RPC_STUB IShockwaveFlash_TGetPropertyAsNumber_Stub(IRpcStubBuffer*    This,
                                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                                              PRPC_MESSAGE       _pRpcMessage,
                                                              DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_SWRemote_Proxy(IShockwaveFlash*          This,
                                                                               /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_SWRemote_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_SWRemote_Proxy(IShockwaveFlash* This,
                                                                                                 /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_SWRemote_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_FlashVars_Proxy(IShockwaveFlash*          This,
                                                                                /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_FlashVars_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_FlashVars_Proxy(IShockwaveFlash* This,
                                                                                                  /* [in] */ BSTR pVal);

    void __RPC_STUB IShockwaveFlash_put_FlashVars_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_AllowScriptAccess_Proxy(IShockwaveFlash*          This,
                                                                                        /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_AllowScriptAccess_Stub(IRpcStubBuffer*    This,
                                                               IRpcChannelBuffer* _pRpcChannelBuffer,
                                                               PRPC_MESSAGE       _pRpcMessage,
                                                               DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_AllowScriptAccess_Proxy(IShockwaveFlash* This,
                                                                                        /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_AllowScriptAccess_Stub(IRpcStubBuffer*    This,
                                                               IRpcChannelBuffer* _pRpcChannelBuffer,
                                                               PRPC_MESSAGE       _pRpcMessage,
                                                               DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_MovieData_Proxy(IShockwaveFlash*          This,
                                                                                /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_MovieData_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_put_MovieData_Proxy(IShockwaveFlash* This,
                                                                                                  /* [in] */ BSTR pVal);

    void __RPC_STUB IShockwaveFlash_put_MovieData_Stub(IRpcStubBuffer*    This,
                                                       IRpcChannelBuffer* _pRpcChannelBuffer,
                                                       PRPC_MESSAGE       _pRpcMessage,
                                                       DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_InlineData_Proxy(IShockwaveFlash*               This,
                                                                                 /* [retval][out] */ IUnknown** ppIUnknown);

    void __RPC_STUB IShockwaveFlash_get_InlineData_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_InlineData_Proxy(IShockwaveFlash*     This,
                                                                                 /* [in] */ IUnknown* ppIUnknown);

    void __RPC_STUB IShockwaveFlash_put_InlineData_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_SeamlessTabbing_Proxy(IShockwaveFlash*                  This,
                                                                                      /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_SeamlessTabbing_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_SeamlessTabbing_Proxy(IShockwaveFlash*        This,
                                                                                      /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_SeamlessTabbing_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_EnforceLocalSecurity_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_EnforceLocalSecurity_Stub(IRpcStubBuffer*    This,
                                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                                              PRPC_MESSAGE       _pRpcMessage,
                                                              DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_Profile_Proxy(IShockwaveFlash*                  This,
                                                                              /* [retval][out] */ VARIANT_BOOL* pVal);

    void __RPC_STUB IShockwaveFlash_get_Profile_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_Profile_Proxy(IShockwaveFlash*        This,
                                                                              /* [in] */ VARIANT_BOOL pVal);

    void __RPC_STUB IShockwaveFlash_put_Profile_Stub(IRpcStubBuffer*    This,
                                                     IRpcChannelBuffer* _pRpcChannelBuffer,
                                                     PRPC_MESSAGE       _pRpcMessage,
                                                     DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_ProfileAddress_Proxy(IShockwaveFlash*          This,
                                                                                     /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_ProfileAddress_Stub(IRpcStubBuffer*    This,
                                                            IRpcChannelBuffer* _pRpcChannelBuffer,
                                                            PRPC_MESSAGE       _pRpcMessage,
                                                            DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_ProfileAddress_Proxy(IShockwaveFlash* This,
                                                                                     /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_ProfileAddress_Stub(IRpcStubBuffer*    This,
                                                            IRpcChannelBuffer* _pRpcChannelBuffer,
                                                            PRPC_MESSAGE       _pRpcMessage,
                                                            DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_ProfilePort_Proxy(IShockwaveFlash*          This,
                                                                                  /* [retval][out] */ long* pVal);

    void __RPC_STUB IShockwaveFlash_get_ProfilePort_Stub(IRpcStubBuffer*    This,
                                                         IRpcChannelBuffer* _pRpcChannelBuffer,
                                                         PRPC_MESSAGE       _pRpcMessage,
                                                         DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_ProfilePort_Proxy(IShockwaveFlash* This,
                                                                                  /* [in] */ long  pVal);

    void __RPC_STUB IShockwaveFlash_put_ProfilePort_Stub(IRpcStubBuffer*    This,
                                                         IRpcChannelBuffer* _pRpcChannelBuffer,
                                                         PRPC_MESSAGE       _pRpcMessage,
                                                         DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE
                                   IShockwaveFlash_CallFunction_Proxy(IShockwaveFlash*          This,
                                                                      /* [in] */ BSTR           request,
                                                                      /* [retval][out] */ BSTR* response);

    void __RPC_STUB IShockwaveFlash_CallFunction_Stub(IRpcStubBuffer*    This,
                                                      IRpcChannelBuffer* _pRpcChannelBuffer,
                                                      PRPC_MESSAGE       _pRpcMessage,
                                                      DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_SetReturnValue_Proxy(IShockwaveFlash* This,
                                                                                          /* [in] */ BSTR  returnValue);

    void __RPC_STUB IShockwaveFlash_SetReturnValue_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    /* [helpstring][id] */ HRESULT STDMETHODCALLTYPE IShockwaveFlash_DisableLocalSecurity_Proxy(IShockwaveFlash* This);

    void __RPC_STUB IShockwaveFlash_DisableLocalSecurity_Stub(IRpcStubBuffer*    This,
                                                              IRpcChannelBuffer* _pRpcChannelBuffer,
                                                              PRPC_MESSAGE       _pRpcMessage,
                                                              DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_AllowNetworking_Proxy(IShockwaveFlash*          This,
                                                                                      /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_AllowNetworking_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_AllowNetworking_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_AllowNetworking_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][propget][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_get_AllowFullScreen_Proxy(IShockwaveFlash*          This,
                                                                                      /* [retval][out] */ BSTR* pVal);

    void __RPC_STUB IShockwaveFlash_get_AllowFullScreen_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

    /* [helpstring][propput][id] */ HRESULT STDMETHODCALLTYPE
                                            IShockwaveFlash_put_AllowFullScreen_Proxy(IShockwaveFlash* This,
                                                                                      /* [in] */ BSTR  pVal);

    void __RPC_STUB IShockwaveFlash_put_AllowFullScreen_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

#endif /* __IShockwaveFlash_INTERFACE_DEFINED__ */

#ifndef ___IShockwaveFlashEvents_DISPINTERFACE_DEFINED__
#define ___IShockwaveFlashEvents_DISPINTERFACE_DEFINED__

    /* dispinterface _IShockwaveFlashEvents */
    /* [hidden][helpstring][uuid] */

    EXTERN_C const IID DIID__IShockwaveFlashEvents;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("D27CDB6D-AE6D-11CF-96B8-444553540000")
    _IShockwaveFlashEvents : public IDispatch{};

#else /* C style interface */

    typedef struct _IShockwaveFlashEventsVtbl
    {
        BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)
        (_IShockwaveFlashEvents*    This,
         /* [in] */ REFIID          riid,
         /* [iid_is][out] */ void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(_IShockwaveFlashEvents* This);

        ULONG(STDMETHODCALLTYPE* Release)(_IShockwaveFlashEvents* This);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfoCount)
        (_IShockwaveFlashEvents* This,
         /* [out] */ UINT*       pctinfo);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfo)
        (_IShockwaveFlashEvents* This,
         /* [in] */ UINT         iTInfo,
         /* [in] */ LCID         lcid,
         /* [out] */ ITypeInfo** ppTInfo);

        HRESULT(STDMETHODCALLTYPE* GetIDsOfNames)
        (_IShockwaveFlashEvents*       This,
         /* [in] */ REFIID             riid,
         /* [size_is][in] */ LPOLESTR* rgszNames,
         /* [in] */ UINT               cNames,
         /* [in] */ LCID               lcid,
         /* [size_is][out] */ DISPID*  rgDispId);

        /* [local] */ HRESULT(STDMETHODCALLTYPE* Invoke)(_IShockwaveFlashEvents*     This,
                                                         /* [in] */ DISPID           dispIdMember,
                                                         /* [in] */ REFIID           riid,
                                                         /* [in] */ LCID             lcid,
                                                         /* [in] */ WORD             wFlags,
                                                         /* [out][in] */ DISPPARAMS* pDispParams,
                                                         /* [out] */ VARIANT*        pVarResult,
                                                         /* [out] */ EXCEPINFO*      pExcepInfo,
                                                         /* [out] */ UINT*           puArgErr);

        END_INTERFACE
    } _IShockwaveFlashEventsVtbl;

    interface _IShockwaveFlashEvents { CONST_VTBL struct _IShockwaveFlashEventsVtbl* lpVtbl; };

#ifdef COBJMACROS

#define _IShockwaveFlashEvents_QueryInterface(This, riid, ppvObject)                                                   \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define _IShockwaveFlashEvents_AddRef(This) (This)->lpVtbl->AddRef(This)

#define _IShockwaveFlashEvents_Release(This) (This)->lpVtbl->Release(This)

#define _IShockwaveFlashEvents_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define _IShockwaveFlashEvents_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                                \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define _IShockwaveFlashEvents_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                            \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define _IShockwaveFlashEvents_Invoke(                                                                                 \
    This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)                             \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)

#endif /* COBJMACROS */

#endif /* C style interface */

#endif /* ___IShockwaveFlashEvents_DISPINTERFACE_DEFINED__ */

#ifndef __IFlashFactory_INTERFACE_DEFINED__
#define __IFlashFactory_INTERFACE_DEFINED__

    /* interface IFlashFactory */
    /* [object][helpstring][uuid] */

    EXTERN_C const IID IID_IFlashFactory;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("D27CDB70-AE6D-11CF-96B8-444553540000")
    IFlashFactory : public IUnknown{public : };

#else /* C style interface */

    typedef struct IFlashFactoryVtbl
    {
        BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)
        (IFlashFactory*             This,
         /* [in] */ REFIID          riid,
         /* [iid_is][out] */ void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(IFlashFactory* This);

        ULONG(STDMETHODCALLTYPE* Release)(IFlashFactory* This);

        END_INTERFACE
    } IFlashFactoryVtbl;

    interface IFlashFactory { CONST_VTBL struct IFlashFactoryVtbl* lpVtbl; };

#ifdef COBJMACROS

#define IFlashFactory_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IFlashFactory_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IFlashFactory_Release(This) (This)->lpVtbl->Release(This)

#endif /* COBJMACROS */

#endif /* C style interface */

#endif /* __IFlashFactory_INTERFACE_DEFINED__ */

#ifndef __IFlashObjectInterface_INTERFACE_DEFINED__
#define __IFlashObjectInterface_INTERFACE_DEFINED__

    /* interface IFlashObjectInterface */
    /* [object][helpstring][uuid] */

    EXTERN_C const IID IID_IFlashObjectInterface;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("D27CDB72-AE6D-11CF-96B8-444553540000")
    IFlashObjectInterface : public IDispatchEx{public : };

#else /* C style interface */

    typedef struct IFlashObjectInterfaceVtbl
    {
        BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)
        (IFlashObjectInterface*     This,
         /* [in] */ REFIID          riid,
         /* [iid_is][out] */ void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(IFlashObjectInterface* This);

        ULONG(STDMETHODCALLTYPE* Release)(IFlashObjectInterface* This);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfoCount)
        (IFlashObjectInterface* This,
         /* [out] */ UINT*      pctinfo);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfo)
        (IFlashObjectInterface*  This,
         /* [in] */ UINT         iTInfo,
         /* [in] */ LCID         lcid,
         /* [out] */ ITypeInfo** ppTInfo);

        HRESULT(STDMETHODCALLTYPE* GetIDsOfNames)
        (IFlashObjectInterface*        This,
         /* [in] */ REFIID             riid,
         /* [size_is][in] */ LPOLESTR* rgszNames,
         /* [in] */ UINT               cNames,
         /* [in] */ LCID               lcid,
         /* [size_is][out] */ DISPID*  rgDispId);

        /* [local] */ HRESULT(STDMETHODCALLTYPE* Invoke)(IFlashObjectInterface*      This,
                                                         /* [in] */ DISPID           dispIdMember,
                                                         /* [in] */ REFIID           riid,
                                                         /* [in] */ LCID             lcid,
                                                         /* [in] */ WORD             wFlags,
                                                         /* [out][in] */ DISPPARAMS* pDispParams,
                                                         /* [out] */ VARIANT*        pVarResult,
                                                         /* [out] */ EXCEPINFO*      pExcepInfo,
                                                         /* [out] */ UINT*           puArgErr);

        HRESULT(__stdcall* GetDispID)
        (IFlashObjectInterface*   This,
         /* [in] */ BSTR          bstrName,
         /* [in] */ unsigned long grfdex,
         /* [out] */ long*        pid);

        HRESULT(__stdcall* RemoteInvokeEx)
        (IFlashObjectInterface*       This,
         /* [in] */ long              id,
         /* [in] */ unsigned long     lcid,
         /* [in] */ unsigned long     dwFlags,
         /* [in] */ DISPPARAMS*       pdp,
         /* [out] */ VARIANT*         pvarRes,
         /* [out] */ EXCEPINFO*       pei,
         /* [in] */ IServiceProvider* pspCaller,
         /* [in] */ unsigned int      cvarRefArg,
         /* [in] */ unsigned int*     rgiRefArg,
         /* [out][in] */ VARIANT*     rgvarRefArg);

        HRESULT(__stdcall* DeleteMemberByName)
        (IFlashObjectInterface*   This,
         /* [in] */ BSTR          bstrName,
         /* [in] */ unsigned long grfdex);

        HRESULT(__stdcall* DeleteMemberByDispID)
        (IFlashObjectInterface* This,
         /* [in] */ long        id);

        HRESULT(__stdcall* GetMemberProperties)
        (IFlashObjectInterface*     This,
         /* [in] */ long            id,
         /* [in] */ unsigned long   grfdexFetch,
         /* [out] */ unsigned long* pgrfdex);

        HRESULT(__stdcall* GetMemberName)
        (IFlashObjectInterface* This,
         /* [in] */ long        id,
         /* [out] */ BSTR*      pbstrName);

        HRESULT(__stdcall* GetNextDispID)
        (IFlashObjectInterface*   This,
         /* [in] */ unsigned long grfdex,
         /* [in] */ long          id,
         /* [out] */ long*        pid);

        HRESULT(__stdcall* GetNameSpaceParent)
        (IFlashObjectInterface* This,
         /* [out] */ IUnknown** ppunk);

        END_INTERFACE
    } IFlashObjectInterfaceVtbl;

    interface IFlashObjectInterface { CONST_VTBL struct IFlashObjectInterfaceVtbl* lpVtbl; };

#ifdef COBJMACROS

#define IFlashObjectInterface_QueryInterface(This, riid, ppvObject)                                                    \
    (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IFlashObjectInterface_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IFlashObjectInterface_Release(This) (This)->lpVtbl->Release(This)

#define IFlashObjectInterface_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IFlashObjectInterface_GetTypeInfo(This, iTInfo, lcid, ppTInfo)                                                 \
    (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IFlashObjectInterface_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                             \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IFlashObjectInterface_Invoke(                                                                                  \
    This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)                             \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)

#define IFlashObjectInterface_GetDispID(This, bstrName, grfdex, pid)                                                   \
    (This)->lpVtbl->GetDispID(This, bstrName, grfdex, pid)

#define IFlashObjectInterface_RemoteInvokeEx(                                                                          \
    This, id, lcid, dwFlags, pdp, pvarRes, pei, pspCaller, cvarRefArg, rgiRefArg, rgvarRefArg)                         \
    (This)->lpVtbl->RemoteInvokeEx(                                                                                    \
        This, id, lcid, dwFlags, pdp, pvarRes, pei, pspCaller, cvarRefArg, rgiRefArg, rgvarRefArg)

#define IFlashObjectInterface_DeleteMemberByName(This, bstrName, grfdex)                                               \
    (This)->lpVtbl->DeleteMemberByName(This, bstrName, grfdex)

#define IFlashObjectInterface_DeleteMemberByDispID(This, id) (This)->lpVtbl->DeleteMemberByDispID(This, id)

#define IFlashObjectInterface_GetMemberProperties(This, id, grfdexFetch, pgrfdex)                                      \
    (This)->lpVtbl->GetMemberProperties(This, id, grfdexFetch, pgrfdex)

#define IFlashObjectInterface_GetMemberName(This, id, pbstrName) (This)->lpVtbl->GetMemberName(This, id, pbstrName)

#define IFlashObjectInterface_GetNextDispID(This, grfdex, id, pid) (This)->lpVtbl->GetNextDispID(This, grfdex, id, pid)

#define IFlashObjectInterface_GetNameSpaceParent(This, ppunk) (This)->lpVtbl->GetNameSpaceParent(This, ppunk)

#endif /* COBJMACROS */

#endif /* C style interface */

#endif /* __IFlashObjectInterface_INTERFACE_DEFINED__ */

#ifndef __IDispatchEx_INTERFACE_DEFINED__
#define __IDispatchEx_INTERFACE_DEFINED__

    /* interface IDispatchEx */
    /* [object][uuid] */

    EXTERN_C const IID IID_IDispatchEx;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("A6EF9860-C720-11D0-9337-00A0C90DCAA9")
    IDispatchEx : public IDispatch
    {
      public:
        virtual HRESULT __stdcall GetDispID(
            /* [in] */ BSTR          bstrName,
            /* [in] */ unsigned long grfdex,
            /* [out] */ long*        pid) = 0;

        virtual HRESULT __stdcall RemoteInvokeEx(
            /* [in] */ long              id,
            /* [in] */ unsigned long     lcid,
            /* [in] */ unsigned long     dwFlags,
            /* [in] */ DISPPARAMS*       pdp,
            /* [out] */ VARIANT*         pvarRes,
            /* [out] */ EXCEPINFO*       pei,
            /* [in] */ IServiceProvider* pspCaller,
            /* [in] */ unsigned int      cvarRefArg,
            /* [in] */ unsigned int*     rgiRefArg,
            /* [out][in] */ VARIANT*     rgvarRefArg) = 0;

        virtual HRESULT __stdcall DeleteMemberByName(
            /* [in] */ BSTR          bstrName,
            /* [in] */ unsigned long grfdex) = 0;

        virtual HRESULT __stdcall DeleteMemberByDispID(
            /* [in] */ long id) = 0;

        virtual HRESULT __stdcall GetMemberProperties(
            /* [in] */ long            id,
            /* [in] */ unsigned long   grfdexFetch,
            /* [out] */ unsigned long* pgrfdex) = 0;

        virtual HRESULT __stdcall GetMemberName(
            /* [in] */ long   id,
            /* [out] */ BSTR* pbstrName) = 0;

        virtual HRESULT __stdcall GetNextDispID(
            /* [in] */ unsigned long grfdex,
            /* [in] */ long          id,
            /* [out] */ long*        pid) = 0;

        virtual HRESULT __stdcall GetNameSpaceParent(
            /* [out] */ IUnknown * *ppunk) = 0;
    };

#else /* C style interface */

    typedef struct IDispatchExVtbl
    {
        BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)
        (IDispatchEx*               This,
         /* [in] */ REFIID          riid,
         /* [iid_is][out] */ void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(IDispatchEx* This);

        ULONG(STDMETHODCALLTYPE* Release)(IDispatchEx* This);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfoCount)
        (IDispatchEx*      This,
         /* [out] */ UINT* pctinfo);

        HRESULT(STDMETHODCALLTYPE* GetTypeInfo)
        (IDispatchEx*            This,
         /* [in] */ UINT         iTInfo,
         /* [in] */ LCID         lcid,
         /* [out] */ ITypeInfo** ppTInfo);

        HRESULT(STDMETHODCALLTYPE* GetIDsOfNames)
        (IDispatchEx*                  This,
         /* [in] */ REFIID             riid,
         /* [size_is][in] */ LPOLESTR* rgszNames,
         /* [in] */ UINT               cNames,
         /* [in] */ LCID               lcid,
         /* [size_is][out] */ DISPID*  rgDispId);

        /* [local] */ HRESULT(STDMETHODCALLTYPE* Invoke)(IDispatchEx*                This,
                                                         /* [in] */ DISPID           dispIdMember,
                                                         /* [in] */ REFIID           riid,
                                                         /* [in] */ LCID             lcid,
                                                         /* [in] */ WORD             wFlags,
                                                         /* [out][in] */ DISPPARAMS* pDispParams,
                                                         /* [out] */ VARIANT*        pVarResult,
                                                         /* [out] */ EXCEPINFO*      pExcepInfo,
                                                         /* [out] */ UINT*           puArgErr);

        HRESULT(__stdcall* GetDispID)
        (IDispatchEx*             This,
         /* [in] */ BSTR          bstrName,
         /* [in] */ unsigned long grfdex,
         /* [out] */ long*        pid);

        HRESULT(__stdcall* RemoteInvokeEx)
        (IDispatchEx*                 This,
         /* [in] */ long              id,
         /* [in] */ unsigned long     lcid,
         /* [in] */ unsigned long     dwFlags,
         /* [in] */ DISPPARAMS*       pdp,
         /* [out] */ VARIANT*         pvarRes,
         /* [out] */ EXCEPINFO*       pei,
         /* [in] */ IServiceProvider* pspCaller,
         /* [in] */ unsigned int      cvarRefArg,
         /* [in] */ unsigned int*     rgiRefArg,
         /* [out][in] */ VARIANT*     rgvarRefArg);

        HRESULT(__stdcall* DeleteMemberByName)
        (IDispatchEx*             This,
         /* [in] */ BSTR          bstrName,
         /* [in] */ unsigned long grfdex);

        HRESULT(__stdcall* DeleteMemberByDispID)
        (IDispatchEx*    This,
         /* [in] */ long id);

        HRESULT(__stdcall* GetMemberProperties)
        (IDispatchEx*               This,
         /* [in] */ long            id,
         /* [in] */ unsigned long   grfdexFetch,
         /* [out] */ unsigned long* pgrfdex);

        HRESULT(__stdcall* GetMemberName)
        (IDispatchEx*      This,
         /* [in] */ long   id,
         /* [out] */ BSTR* pbstrName);

        HRESULT(__stdcall* GetNextDispID)
        (IDispatchEx*             This,
         /* [in] */ unsigned long grfdex,
         /* [in] */ long          id,
         /* [out] */ long*        pid);

        HRESULT(__stdcall* GetNameSpaceParent)
        (IDispatchEx*           This,
         /* [out] */ IUnknown** ppunk);

        END_INTERFACE
    } IDispatchExVtbl;

    interface IDispatchEx { CONST_VTBL struct IDispatchExVtbl* lpVtbl; };

#ifdef COBJMACROS

#define IDispatchEx_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IDispatchEx_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IDispatchEx_Release(This) (This)->lpVtbl->Release(This)

#define IDispatchEx_GetTypeInfoCount(This, pctinfo) (This)->lpVtbl->GetTypeInfoCount(This, pctinfo)

#define IDispatchEx_GetTypeInfo(This, iTInfo, lcid, ppTInfo) (This)->lpVtbl->GetTypeInfo(This, iTInfo, lcid, ppTInfo)

#define IDispatchEx_GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)                                       \
    (This)->lpVtbl->GetIDsOfNames(This, riid, rgszNames, cNames, lcid, rgDispId)

#define IDispatchEx_Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)      \
    (This)->lpVtbl->Invoke(This, dispIdMember, riid, lcid, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr)

#define IDispatchEx_GetDispID(This, bstrName, grfdex, pid) (This)->lpVtbl->GetDispID(This, bstrName, grfdex, pid)

#define IDispatchEx_RemoteInvokeEx(                                                                                    \
    This, id, lcid, dwFlags, pdp, pvarRes, pei, pspCaller, cvarRefArg, rgiRefArg, rgvarRefArg)                         \
    (This)->lpVtbl->RemoteInvokeEx(                                                                                    \
        This, id, lcid, dwFlags, pdp, pvarRes, pei, pspCaller, cvarRefArg, rgiRefArg, rgvarRefArg)

#define IDispatchEx_DeleteMemberByName(This, bstrName, grfdex)                                                         \
    (This)->lpVtbl->DeleteMemberByName(This, bstrName, grfdex)

#define IDispatchEx_DeleteMemberByDispID(This, id) (This)->lpVtbl->DeleteMemberByDispID(This, id)

#define IDispatchEx_GetMemberProperties(This, id, grfdexFetch, pgrfdex)                                                \
    (This)->lpVtbl->GetMemberProperties(This, id, grfdexFetch, pgrfdex)

#define IDispatchEx_GetMemberName(This, id, pbstrName) (This)->lpVtbl->GetMemberName(This, id, pbstrName)

#define IDispatchEx_GetNextDispID(This, grfdex, id, pid) (This)->lpVtbl->GetNextDispID(This, grfdex, id, pid)

#define IDispatchEx_GetNameSpaceParent(This, ppunk) (This)->lpVtbl->GetNameSpaceParent(This, ppunk)

#endif /* COBJMACROS */

#endif /* C style interface */

    HRESULT __stdcall IDispatchEx_GetDispID_Proxy(IDispatchEx*             This,
                                                  /* [in] */ BSTR          bstrName,
                                                  /* [in] */ unsigned long grfdex,
                                                  /* [out] */ long*        pid);

    void __RPC_STUB IDispatchEx_GetDispID_Stub(IRpcStubBuffer*    This,
                                               IRpcChannelBuffer* _pRpcChannelBuffer,
                                               PRPC_MESSAGE       _pRpcMessage,
                                               DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_RemoteInvokeEx_Proxy(IDispatchEx*                 This,
                                                       /* [in] */ long              id,
                                                       /* [in] */ unsigned long     lcid,
                                                       /* [in] */ unsigned long     dwFlags,
                                                       /* [in] */ DISPPARAMS*       pdp,
                                                       /* [out] */ VARIANT*         pvarRes,
                                                       /* [out] */ EXCEPINFO*       pei,
                                                       /* [in] */ IServiceProvider* pspCaller,
                                                       /* [in] */ unsigned int      cvarRefArg,
                                                       /* [in] */ unsigned int*     rgiRefArg,
                                                       /* [out][in] */ VARIANT*     rgvarRefArg);

    void __RPC_STUB IDispatchEx_RemoteInvokeEx_Stub(IRpcStubBuffer*    This,
                                                    IRpcChannelBuffer* _pRpcChannelBuffer,
                                                    PRPC_MESSAGE       _pRpcMessage,
                                                    DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_DeleteMemberByName_Proxy(IDispatchEx*             This,
                                                           /* [in] */ BSTR          bstrName,
                                                           /* [in] */ unsigned long grfdex);

    void __RPC_STUB IDispatchEx_DeleteMemberByName_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_DeleteMemberByDispID_Proxy(IDispatchEx*    This,
                                                             /* [in] */ long id);

    void __RPC_STUB IDispatchEx_DeleteMemberByDispID_Stub(IRpcStubBuffer*    This,
                                                          IRpcChannelBuffer* _pRpcChannelBuffer,
                                                          PRPC_MESSAGE       _pRpcMessage,
                                                          DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_GetMemberProperties_Proxy(IDispatchEx*               This,
                                                            /* [in] */ long            id,
                                                            /* [in] */ unsigned long   grfdexFetch,
                                                            /* [out] */ unsigned long* pgrfdex);

    void __RPC_STUB IDispatchEx_GetMemberProperties_Stub(IRpcStubBuffer*    This,
                                                         IRpcChannelBuffer* _pRpcChannelBuffer,
                                                         PRPC_MESSAGE       _pRpcMessage,
                                                         DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_GetMemberName_Proxy(IDispatchEx*      This,
                                                      /* [in] */ long   id,
                                                      /* [out] */ BSTR* pbstrName);

    void __RPC_STUB IDispatchEx_GetMemberName_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_GetNextDispID_Proxy(IDispatchEx*             This,
                                                      /* [in] */ unsigned long grfdex,
                                                      /* [in] */ long          id,
                                                      /* [out] */ long*        pid);

    void __RPC_STUB IDispatchEx_GetNextDispID_Stub(IRpcStubBuffer*    This,
                                                   IRpcChannelBuffer* _pRpcChannelBuffer,
                                                   PRPC_MESSAGE       _pRpcMessage,
                                                   DWORD*             _pdwStubPhase);

    HRESULT __stdcall IDispatchEx_GetNameSpaceParent_Proxy(IDispatchEx*           This,
                                                           /* [out] */ IUnknown** ppunk);

    void __RPC_STUB IDispatchEx_GetNameSpaceParent_Stub(IRpcStubBuffer*    This,
                                                        IRpcChannelBuffer* _pRpcChannelBuffer,
                                                        PRPC_MESSAGE       _pRpcMessage,
                                                        DWORD*             _pdwStubPhase);

#endif /* __IDispatchEx_INTERFACE_DEFINED__ */

#ifndef __IServiceProvider_INTERFACE_DEFINED__
#define __IServiceProvider_INTERFACE_DEFINED__

    /* interface IServiceProvider */
    /* [object][uuid] */

    EXTERN_C const IID IID_IServiceProvider;

#if defined(__cplusplus) && !defined(CINTERFACE)

    MIDL_INTERFACE("6D5140C1-7436-11CE-8034-00AA006009FA")
    IServiceProvider : public IUnknown
    {
      public:
        virtual HRESULT __stdcall RemoteQueryService(
            /* [in] */ GUID * guidService,
            /* [in] */ GUID * riid,
            /* [out] */ IUnknown * *ppvObject) = 0;
    };

#else /* C style interface */

    typedef struct IServiceProviderVtbl
    {
        BEGIN_INTERFACE

        HRESULT(STDMETHODCALLTYPE* QueryInterface)
        (IServiceProvider*          This,
         /* [in] */ REFIID          riid,
         /* [iid_is][out] */ void** ppvObject);

        ULONG(STDMETHODCALLTYPE* AddRef)(IServiceProvider* This);

        ULONG(STDMETHODCALLTYPE* Release)(IServiceProvider* This);

        HRESULT(__stdcall* RemoteQueryService)
        (IServiceProvider*      This,
         /* [in] */ GUID*       guidService,
         /* [in] */ GUID*       riid,
         /* [out] */ IUnknown** ppvObject);

        END_INTERFACE
    } IServiceProviderVtbl;

    interface IServiceProvider { CONST_VTBL struct IServiceProviderVtbl* lpVtbl; };

#ifdef COBJMACROS

#define IServiceProvider_QueryInterface(This, riid, ppvObject) (This)->lpVtbl->QueryInterface(This, riid, ppvObject)

#define IServiceProvider_AddRef(This) (This)->lpVtbl->AddRef(This)

#define IServiceProvider_Release(This) (This)->lpVtbl->Release(This)

#define IServiceProvider_RemoteQueryService(This, guidService, riid, ppvObject)                                        \
    (This)->lpVtbl->RemoteQueryService(This, guidService, riid, ppvObject)

#endif /* COBJMACROS */

#endif /* C style interface */

    HRESULT __stdcall IServiceProvider_RemoteQueryService_Proxy(IServiceProvider*      This,
                                                                /* [in] */ GUID*       guidService,
                                                                /* [in] */ GUID*       riid,
                                                                /* [out] */ IUnknown** ppvObject);

    void __RPC_STUB IServiceProvider_RemoteQueryService_Stub(IRpcStubBuffer*    This,
                                                             IRpcChannelBuffer* _pRpcChannelBuffer,
                                                             PRPC_MESSAGE       _pRpcMessage,
                                                             DWORD*             _pdwStubPhase);

#endif /* __IServiceProvider_INTERFACE_DEFINED__ */

    EXTERN_C const CLSID CLSID_ShockwaveFlash;

#ifdef __cplusplus

    class DECLSPEC_UUID("D27CDB6E-AE6D-11CF-96B8-444553540000") ShockwaveFlash;
#endif

    EXTERN_C const CLSID CLSID_FlashObjectInterface;

#ifdef __cplusplus

    class DECLSPEC_UUID("D27CDB71-AE6D-11CF-96B8-444553540000") FlashObjectInterface;
#endif
#endif /* __ShockwaveFlashObjects_LIBRARY_DEFINED__ */

    /* Additional Prototypes for ALL interfaces */

    /* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif
