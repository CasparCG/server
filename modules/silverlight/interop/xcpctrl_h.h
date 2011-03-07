

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 7.00.0555 */
/* at Sat Jan 22 19:55:15 2011
 */
/* Compiler settings for xcpctrl.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 7.00.0555 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif // __RPCNDR_H_VERSION__

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __xcpctrl_h_h__
#define __xcpctrl_h_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

/* Forward Declarations */ 

#ifndef __IXcpObject_FWD_DEFINED__
#define __IXcpObject_FWD_DEFINED__
typedef interface IXcpObject IXcpObject;
#endif 	/* __IXcpObject_FWD_DEFINED__ */


#ifndef __IXcpControl_FWD_DEFINED__
#define __IXcpControl_FWD_DEFINED__
typedef interface IXcpControl IXcpControl;
#endif 	/* __IXcpControl_FWD_DEFINED__ */


#ifndef __IXcpControl2_FWD_DEFINED__
#define __IXcpControl2_FWD_DEFINED__
typedef interface IXcpControl2 IXcpControl2;
#endif 	/* __IXcpControl2_FWD_DEFINED__ */


#ifndef __IXcpControlDownloadCallback_FWD_DEFINED__
#define __IXcpControlDownloadCallback_FWD_DEFINED__
typedef interface IXcpControlDownloadCallback IXcpControlDownloadCallback;
#endif 	/* __IXcpControlDownloadCallback_FWD_DEFINED__ */


#ifndef __IXcpControlHost_FWD_DEFINED__
#define __IXcpControlHost_FWD_DEFINED__
typedef interface IXcpControlHost IXcpControlHost;
#endif 	/* __IXcpControlHost_FWD_DEFINED__ */


#ifndef __IXcpControlHost2_FWD_DEFINED__
#define __IXcpControlHost2_FWD_DEFINED__
typedef interface IXcpControlHost2 IXcpControlHost2;
#endif 	/* __IXcpControlHost2_FWD_DEFINED__ */


#ifndef __IXcpControlHost3_FWD_DEFINED__
#define __IXcpControlHost3_FWD_DEFINED__
typedef interface IXcpControlHost3 IXcpControlHost3;
#endif 	/* __IXcpControlHost3_FWD_DEFINED__ */


#ifndef __XcpControl_FWD_DEFINED__
#define __XcpControl_FWD_DEFINED__

#ifdef __cplusplus
typedef class XcpControl XcpControl;
#else
typedef struct XcpControl XcpControl;
#endif /* __cplusplus */

#endif 	/* __XcpControl_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IXcpObject_INTERFACE_DEFINED__
#define __IXcpObject_INTERFACE_DEFINED__

/* interface IXcpObject */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IXcpObject;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("EE38D0F1-5AE3-408c-A6BF-8410E645F376")
    IXcpObject : public IDispatch
    {
    public:
    };
    
#else 	/* C style interface */

    typedef struct IXcpObjectVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpObject * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpObject * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpObject * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IXcpObject * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IXcpObject * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IXcpObject * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IXcpObject * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        END_INTERFACE
    } IXcpObjectVtbl;

    interface IXcpObject
    {
        CONST_VTBL struct IXcpObjectVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpObject_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpObject_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpObject_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpObject_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IXcpObject_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IXcpObject_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IXcpObject_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpObject_INTERFACE_DEFINED__ */


#ifndef __IXcpControl_INTERFACE_DEFINED__
#define __IXcpControl_INTERFACE_DEFINED__

/* interface IXcpControl */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IXcpControl;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1FB839CC-116C-4C9B-AE8E-3DBB6496E326")
    IXcpControl : public IDispatch
    {
    public:
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_Source( 
            /* [retval][out] */ BSTR *pstr) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_Source( 
            /* [in] */ BSTR str) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_IsLoaded( 
            /* [retval][out] */ VARIANT_BOOL *pb) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_Content( 
            /* [retval][out] */ IDispatch **ppContent) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_Settings( 
            /* [retval][out] */ IDispatch **ppSettings) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_OnLoad( 
            /* [retval][out] */ VARIANT *pVAR) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_OnLoad( 
            /* [in] */ VARIANT *pVAR) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_OnError( 
            /* [retval][out] */ VARIANT *pVAR) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_OnError( 
            /* [in] */ VARIANT *pVAR) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE CreateObject( 
            /* [in] */ BSTR id,
            /* [retval][out] */ IXcpObject **ppDisp) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE IsVersionSupported( 
            /* [in] */ BSTR v,
            /* [retval][out] */ VARIANT_BOOL *pb) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_InitParams( 
            /* [retval][out] */ BSTR *initparams) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_InitParams( 
            /* [in] */ BSTR initparams) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXcpControlVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpControl * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpControl * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpControl * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IXcpControl * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IXcpControl * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IXcpControl * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IXcpControl * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            IXcpControl * This,
            /* [retval][out] */ BSTR *pstr);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            IXcpControl * This,
            /* [in] */ BSTR str);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsLoaded )( 
            IXcpControl * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Content )( 
            IXcpControl * This,
            /* [retval][out] */ IDispatch **ppContent);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Settings )( 
            IXcpControl * This,
            /* [retval][out] */ IDispatch **ppSettings);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OnLoad )( 
            IXcpControl * This,
            /* [retval][out] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnLoad )( 
            IXcpControl * This,
            /* [in] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OnError )( 
            IXcpControl * This,
            /* [retval][out] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnError )( 
            IXcpControl * This,
            /* [in] */ VARIANT *pVAR);
        
        HRESULT ( STDMETHODCALLTYPE *CreateObject )( 
            IXcpControl * This,
            /* [in] */ BSTR id,
            /* [retval][out] */ IXcpObject **ppDisp);
        
        HRESULT ( STDMETHODCALLTYPE *IsVersionSupported )( 
            IXcpControl * This,
            /* [in] */ BSTR v,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InitParams )( 
            IXcpControl * This,
            /* [retval][out] */ BSTR *initparams);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InitParams )( 
            IXcpControl * This,
            /* [in] */ BSTR initparams);
        
        END_INTERFACE
    } IXcpControlVtbl;

    interface IXcpControl
    {
        CONST_VTBL struct IXcpControlVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpControl_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpControl_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpControl_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpControl_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IXcpControl_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IXcpControl_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IXcpControl_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IXcpControl_get_Source(This,pstr)	\
    ( (This)->lpVtbl -> get_Source(This,pstr) ) 

#define IXcpControl_put_Source(This,str)	\
    ( (This)->lpVtbl -> put_Source(This,str) ) 

#define IXcpControl_get_IsLoaded(This,pb)	\
    ( (This)->lpVtbl -> get_IsLoaded(This,pb) ) 

#define IXcpControl_get_Content(This,ppContent)	\
    ( (This)->lpVtbl -> get_Content(This,ppContent) ) 

#define IXcpControl_get_Settings(This,ppSettings)	\
    ( (This)->lpVtbl -> get_Settings(This,ppSettings) ) 

#define IXcpControl_get_OnLoad(This,pVAR)	\
    ( (This)->lpVtbl -> get_OnLoad(This,pVAR) ) 

#define IXcpControl_put_OnLoad(This,pVAR)	\
    ( (This)->lpVtbl -> put_OnLoad(This,pVAR) ) 

#define IXcpControl_get_OnError(This,pVAR)	\
    ( (This)->lpVtbl -> get_OnError(This,pVAR) ) 

#define IXcpControl_put_OnError(This,pVAR)	\
    ( (This)->lpVtbl -> put_OnError(This,pVAR) ) 

#define IXcpControl_CreateObject(This,id,ppDisp)	\
    ( (This)->lpVtbl -> CreateObject(This,id,ppDisp) ) 

#define IXcpControl_IsVersionSupported(This,v,pb)	\
    ( (This)->lpVtbl -> IsVersionSupported(This,v,pb) ) 

#define IXcpControl_get_InitParams(This,initparams)	\
    ( (This)->lpVtbl -> get_InitParams(This,initparams) ) 

#define IXcpControl_put_InitParams(This,initparams)	\
    ( (This)->lpVtbl -> put_InitParams(This,initparams) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpControl_INTERFACE_DEFINED__ */


#ifndef __IXcpControl2_INTERFACE_DEFINED__
#define __IXcpControl2_INTERFACE_DEFINED__

/* interface IXcpControl2 */
/* [unique][helpstring][nonextensible][dual][uuid][object] */ 


EXTERN_C const IID IID_IXcpControl2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1c3294f9-891f-49b1-bbae-492a68ba10cc")
    IXcpControl2 : public IXcpControl
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE LoadRuntime( void) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_SplashScreenSource( 
            /* [retval][out] */ BSTR *pstr) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_SplashScreenSource( 
            /* [in] */ BSTR str) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_OnSourceDownloadComplete( 
            /* [retval][out] */ VARIANT *pVAR) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_OnSourceDownloadComplete( 
            /* [in] */ VARIANT *pVAR) = 0;
        
        virtual /* [requestedit][bindable][propget] */ HRESULT STDMETHODCALLTYPE get_OnSourceDownloadProgressChanged( 
            /* [retval][out] */ VARIANT *pVAR) = 0;
        
        virtual /* [requestedit][bindable][propput] */ HRESULT STDMETHODCALLTYPE put_OnSourceDownloadProgressChanged( 
            /* [in] */ VARIANT *pVAR) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXcpControl2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpControl2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpControl2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpControl2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfoCount )( 
            IXcpControl2 * This,
            /* [out] */ UINT *pctinfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetTypeInfo )( 
            IXcpControl2 * This,
            /* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo **ppTInfo);
        
        HRESULT ( STDMETHODCALLTYPE *GetIDsOfNames )( 
            IXcpControl2 * This,
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [range][in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
        
        /* [local] */ HRESULT ( STDMETHODCALLTYPE *Invoke )( 
            IXcpControl2 * This,
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS *pDispParams,
            /* [out] */ VARIANT *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Source )( 
            IXcpControl2 * This,
            /* [retval][out] */ BSTR *pstr);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_Source )( 
            IXcpControl2 * This,
            /* [in] */ BSTR str);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_IsLoaded )( 
            IXcpControl2 * This,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Content )( 
            IXcpControl2 * This,
            /* [retval][out] */ IDispatch **ppContent);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_Settings )( 
            IXcpControl2 * This,
            /* [retval][out] */ IDispatch **ppSettings);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OnLoad )( 
            IXcpControl2 * This,
            /* [retval][out] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnLoad )( 
            IXcpControl2 * This,
            /* [in] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OnError )( 
            IXcpControl2 * This,
            /* [retval][out] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnError )( 
            IXcpControl2 * This,
            /* [in] */ VARIANT *pVAR);
        
        HRESULT ( STDMETHODCALLTYPE *CreateObject )( 
            IXcpControl2 * This,
            /* [in] */ BSTR id,
            /* [retval][out] */ IXcpObject **ppDisp);
        
        HRESULT ( STDMETHODCALLTYPE *IsVersionSupported )( 
            IXcpControl2 * This,
            /* [in] */ BSTR v,
            /* [retval][out] */ VARIANT_BOOL *pb);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_InitParams )( 
            IXcpControl2 * This,
            /* [retval][out] */ BSTR *initparams);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_InitParams )( 
            IXcpControl2 * This,
            /* [in] */ BSTR initparams);
        
        HRESULT ( STDMETHODCALLTYPE *LoadRuntime )( 
            IXcpControl2 * This);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_SplashScreenSource )( 
            IXcpControl2 * This,
            /* [retval][out] */ BSTR *pstr);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_SplashScreenSource )( 
            IXcpControl2 * This,
            /* [in] */ BSTR str);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OnSourceDownloadComplete )( 
            IXcpControl2 * This,
            /* [retval][out] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnSourceDownloadComplete )( 
            IXcpControl2 * This,
            /* [in] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propget] */ HRESULT ( STDMETHODCALLTYPE *get_OnSourceDownloadProgressChanged )( 
            IXcpControl2 * This,
            /* [retval][out] */ VARIANT *pVAR);
        
        /* [requestedit][bindable][propput] */ HRESULT ( STDMETHODCALLTYPE *put_OnSourceDownloadProgressChanged )( 
            IXcpControl2 * This,
            /* [in] */ VARIANT *pVAR);
        
        END_INTERFACE
    } IXcpControl2Vtbl;

    interface IXcpControl2
    {
        CONST_VTBL struct IXcpControl2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpControl2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpControl2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpControl2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpControl2_GetTypeInfoCount(This,pctinfo)	\
    ( (This)->lpVtbl -> GetTypeInfoCount(This,pctinfo) ) 

#define IXcpControl2_GetTypeInfo(This,iTInfo,lcid,ppTInfo)	\
    ( (This)->lpVtbl -> GetTypeInfo(This,iTInfo,lcid,ppTInfo) ) 

#define IXcpControl2_GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId)	\
    ( (This)->lpVtbl -> GetIDsOfNames(This,riid,rgszNames,cNames,lcid,rgDispId) ) 

#define IXcpControl2_Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr)	\
    ( (This)->lpVtbl -> Invoke(This,dispIdMember,riid,lcid,wFlags,pDispParams,pVarResult,pExcepInfo,puArgErr) ) 


#define IXcpControl2_get_Source(This,pstr)	\
    ( (This)->lpVtbl -> get_Source(This,pstr) ) 

#define IXcpControl2_put_Source(This,str)	\
    ( (This)->lpVtbl -> put_Source(This,str) ) 

#define IXcpControl2_get_IsLoaded(This,pb)	\
    ( (This)->lpVtbl -> get_IsLoaded(This,pb) ) 

#define IXcpControl2_get_Content(This,ppContent)	\
    ( (This)->lpVtbl -> get_Content(This,ppContent) ) 

#define IXcpControl2_get_Settings(This,ppSettings)	\
    ( (This)->lpVtbl -> get_Settings(This,ppSettings) ) 

#define IXcpControl2_get_OnLoad(This,pVAR)	\
    ( (This)->lpVtbl -> get_OnLoad(This,pVAR) ) 

#define IXcpControl2_put_OnLoad(This,pVAR)	\
    ( (This)->lpVtbl -> put_OnLoad(This,pVAR) ) 

#define IXcpControl2_get_OnError(This,pVAR)	\
    ( (This)->lpVtbl -> get_OnError(This,pVAR) ) 

#define IXcpControl2_put_OnError(This,pVAR)	\
    ( (This)->lpVtbl -> put_OnError(This,pVAR) ) 

#define IXcpControl2_CreateObject(This,id,ppDisp)	\
    ( (This)->lpVtbl -> CreateObject(This,id,ppDisp) ) 

#define IXcpControl2_IsVersionSupported(This,v,pb)	\
    ( (This)->lpVtbl -> IsVersionSupported(This,v,pb) ) 

#define IXcpControl2_get_InitParams(This,initparams)	\
    ( (This)->lpVtbl -> get_InitParams(This,initparams) ) 

#define IXcpControl2_put_InitParams(This,initparams)	\
    ( (This)->lpVtbl -> put_InitParams(This,initparams) ) 


#define IXcpControl2_LoadRuntime(This)	\
    ( (This)->lpVtbl -> LoadRuntime(This) ) 

#define IXcpControl2_get_SplashScreenSource(This,pstr)	\
    ( (This)->lpVtbl -> get_SplashScreenSource(This,pstr) ) 

#define IXcpControl2_put_SplashScreenSource(This,str)	\
    ( (This)->lpVtbl -> put_SplashScreenSource(This,str) ) 

#define IXcpControl2_get_OnSourceDownloadComplete(This,pVAR)	\
    ( (This)->lpVtbl -> get_OnSourceDownloadComplete(This,pVAR) ) 

#define IXcpControl2_put_OnSourceDownloadComplete(This,pVAR)	\
    ( (This)->lpVtbl -> put_OnSourceDownloadComplete(This,pVAR) ) 

#define IXcpControl2_get_OnSourceDownloadProgressChanged(This,pVAR)	\
    ( (This)->lpVtbl -> get_OnSourceDownloadProgressChanged(This,pVAR) ) 

#define IXcpControl2_put_OnSourceDownloadProgressChanged(This,pVAR)	\
    ( (This)->lpVtbl -> put_OnSourceDownloadProgressChanged(This,pVAR) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpControl2_INTERFACE_DEFINED__ */


#ifndef __IXcpControlDownloadCallback_INTERFACE_DEFINED__
#define __IXcpControlDownloadCallback_INTERFACE_DEFINED__

/* interface IXcpControlDownloadCallback */
/* [unique][helpstring][nonextensible][uuid][object] */ 


EXTERN_C const IID IID_IXcpControlDownloadCallback;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("2E340632-5D5A-427b-AC31-303F6E32B9E8")
    IXcpControlDownloadCallback : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE OnUrlDownloaded( 
            HRESULT hr,
            IStream *pStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXcpControlDownloadCallbackVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpControlDownloadCallback * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpControlDownloadCallback * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpControlDownloadCallback * This);
        
        HRESULT ( STDMETHODCALLTYPE *OnUrlDownloaded )( 
            IXcpControlDownloadCallback * This,
            HRESULT hr,
            IStream *pStream);
        
        END_INTERFACE
    } IXcpControlDownloadCallbackVtbl;

    interface IXcpControlDownloadCallback
    {
        CONST_VTBL struct IXcpControlDownloadCallbackVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpControlDownloadCallback_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpControlDownloadCallback_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpControlDownloadCallback_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpControlDownloadCallback_OnUrlDownloaded(This,hr,pStream)	\
    ( (This)->lpVtbl -> OnUrlDownloaded(This,hr,pStream) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpControlDownloadCallback_INTERFACE_DEFINED__ */


#ifndef __IXcpControlHost_INTERFACE_DEFINED__
#define __IXcpControlHost_INTERFACE_DEFINED__

/* interface IXcpControlHost */
/* [unique][helpstring][nonextensible][uuid][object] */ 

typedef /* [public] */ 
enum __MIDL_IXcpControlHost_0001
    {	XcpHostOption_FreezeOnInitialFrame	= 0x1,
	XcpHostOption_DisableFullScreen	= 0x2,
	XcpHostOption_DisableManagedExecution	= 0x8,
	XcpHostOption_EnableCrossDomainDownloads	= 0x10,
	XcpHostOption_UseCustomAppDomain	= 0x20,
	XcpHostOption_DisableNetworking	= 0x40,
	XcpHostOption_DisableScriptCallouts	= 0x80,
	XcpHostOption_EnableHtmlDomAccess	= 0x100,
	XcpHostOption_EnableScriptableObjectAccess	= 0x200,
	XcpHostOption_EnableAssemblySharing	= 0x800,
	XcpHostOption_HookGetComAutomationObject	= 0x1000,
	XcpHostOption_EnableElevatedPermissions	= 0x2000,
	XcpHostOption_EnableWindowlessAccessibility	= 0x4000
    } 	XcpHostOptions;


EXTERN_C const IID IID_IXcpControlHost;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("1B36028E-B491-4bb2-8584-8A9E0A677D6E")
    IXcpControlHost : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetHostOptions( 
            /* [retval][out] */ DWORD *pdwOptions) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyLoaded( void) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE NotifyError( 
            /* [in] */ BSTR bstrError,
            /* [in] */ BSTR bstrSource,
            /* [in] */ long nLine,
            /* [in] */ long nColumn) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE InvokeHandler( 
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varArg1,
            /* [in] */ VARIANT varArg2,
            /* [retval][out] */ VARIANT *pvarResult) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetBaseUrl( 
            /* [retval][out] */ BSTR *pbstrUrl) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetNamedSource( 
            /* [in] */ BSTR bstrSourceName,
            /* [retval][out] */ BSTR *pbstrSource) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DownloadUrl( 
            /* [in] */ BSTR bstrUrl,
            /* [in] */ IXcpControlDownloadCallback *pCallback,
            /* [retval][out] */ IStream **ppStream) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXcpControlHostVtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpControlHost * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpControlHost * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpControlHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostOptions )( 
            IXcpControlHost * This,
            /* [retval][out] */ DWORD *pdwOptions);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyLoaded )( 
            IXcpControlHost * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyError )( 
            IXcpControlHost * This,
            /* [in] */ BSTR bstrError,
            /* [in] */ BSTR bstrSource,
            /* [in] */ long nLine,
            /* [in] */ long nColumn);
        
        HRESULT ( STDMETHODCALLTYPE *InvokeHandler )( 
            IXcpControlHost * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varArg1,
            /* [in] */ VARIANT varArg2,
            /* [retval][out] */ VARIANT *pvarResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaseUrl )( 
            IXcpControlHost * This,
            /* [retval][out] */ BSTR *pbstrUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamedSource )( 
            IXcpControlHost * This,
            /* [in] */ BSTR bstrSourceName,
            /* [retval][out] */ BSTR *pbstrSource);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadUrl )( 
            IXcpControlHost * This,
            /* [in] */ BSTR bstrUrl,
            /* [in] */ IXcpControlDownloadCallback *pCallback,
            /* [retval][out] */ IStream **ppStream);
        
        END_INTERFACE
    } IXcpControlHostVtbl;

    interface IXcpControlHost
    {
        CONST_VTBL struct IXcpControlHostVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpControlHost_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpControlHost_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpControlHost_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpControlHost_GetHostOptions(This,pdwOptions)	\
    ( (This)->lpVtbl -> GetHostOptions(This,pdwOptions) ) 

#define IXcpControlHost_NotifyLoaded(This)	\
    ( (This)->lpVtbl -> NotifyLoaded(This) ) 

#define IXcpControlHost_NotifyError(This,bstrError,bstrSource,nLine,nColumn)	\
    ( (This)->lpVtbl -> NotifyError(This,bstrError,bstrSource,nLine,nColumn) ) 

#define IXcpControlHost_InvokeHandler(This,bstrName,varArg1,varArg2,pvarResult)	\
    ( (This)->lpVtbl -> InvokeHandler(This,bstrName,varArg1,varArg2,pvarResult) ) 

#define IXcpControlHost_GetBaseUrl(This,pbstrUrl)	\
    ( (This)->lpVtbl -> GetBaseUrl(This,pbstrUrl) ) 

#define IXcpControlHost_GetNamedSource(This,bstrSourceName,pbstrSource)	\
    ( (This)->lpVtbl -> GetNamedSource(This,bstrSourceName,pbstrSource) ) 

#define IXcpControlHost_DownloadUrl(This,bstrUrl,pCallback,ppStream)	\
    ( (This)->lpVtbl -> DownloadUrl(This,bstrUrl,pCallback,ppStream) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpControlHost_INTERFACE_DEFINED__ */


#ifndef __IXcpControlHost2_INTERFACE_DEFINED__
#define __IXcpControlHost2_INTERFACE_DEFINED__

/* interface IXcpControlHost2 */
/* [unique][helpstring][nonextensible][uuid][object] */ 


EXTERN_C const IID IID_IXcpControlHost2;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("fb3ed7c4-5797-4b44-8695-0c512ea27d8f")
    IXcpControlHost2 : public IXcpControlHost
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetCustomAppDomain( 
            /* [retval][out] */ IUnknown **ppAppDomain) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetControlVersion( 
            /* [out] */ UINT *puMajorVersion,
            /* [out] */ UINT *puMinorVersion) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXcpControlHost2Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpControlHost2 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpControlHost2 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpControlHost2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostOptions )( 
            IXcpControlHost2 * This,
            /* [retval][out] */ DWORD *pdwOptions);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyLoaded )( 
            IXcpControlHost2 * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyError )( 
            IXcpControlHost2 * This,
            /* [in] */ BSTR bstrError,
            /* [in] */ BSTR bstrSource,
            /* [in] */ long nLine,
            /* [in] */ long nColumn);
        
        HRESULT ( STDMETHODCALLTYPE *InvokeHandler )( 
            IXcpControlHost2 * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varArg1,
            /* [in] */ VARIANT varArg2,
            /* [retval][out] */ VARIANT *pvarResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaseUrl )( 
            IXcpControlHost2 * This,
            /* [retval][out] */ BSTR *pbstrUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamedSource )( 
            IXcpControlHost2 * This,
            /* [in] */ BSTR bstrSourceName,
            /* [retval][out] */ BSTR *pbstrSource);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadUrl )( 
            IXcpControlHost2 * This,
            /* [in] */ BSTR bstrUrl,
            /* [in] */ IXcpControlDownloadCallback *pCallback,
            /* [retval][out] */ IStream **ppStream);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomAppDomain )( 
            IXcpControlHost2 * This,
            /* [retval][out] */ IUnknown **ppAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *GetControlVersion )( 
            IXcpControlHost2 * This,
            /* [out] */ UINT *puMajorVersion,
            /* [out] */ UINT *puMinorVersion);
        
        END_INTERFACE
    } IXcpControlHost2Vtbl;

    interface IXcpControlHost2
    {
        CONST_VTBL struct IXcpControlHost2Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpControlHost2_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpControlHost2_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpControlHost2_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpControlHost2_GetHostOptions(This,pdwOptions)	\
    ( (This)->lpVtbl -> GetHostOptions(This,pdwOptions) ) 

#define IXcpControlHost2_NotifyLoaded(This)	\
    ( (This)->lpVtbl -> NotifyLoaded(This) ) 

#define IXcpControlHost2_NotifyError(This,bstrError,bstrSource,nLine,nColumn)	\
    ( (This)->lpVtbl -> NotifyError(This,bstrError,bstrSource,nLine,nColumn) ) 

#define IXcpControlHost2_InvokeHandler(This,bstrName,varArg1,varArg2,pvarResult)	\
    ( (This)->lpVtbl -> InvokeHandler(This,bstrName,varArg1,varArg2,pvarResult) ) 

#define IXcpControlHost2_GetBaseUrl(This,pbstrUrl)	\
    ( (This)->lpVtbl -> GetBaseUrl(This,pbstrUrl) ) 

#define IXcpControlHost2_GetNamedSource(This,bstrSourceName,pbstrSource)	\
    ( (This)->lpVtbl -> GetNamedSource(This,bstrSourceName,pbstrSource) ) 

#define IXcpControlHost2_DownloadUrl(This,bstrUrl,pCallback,ppStream)	\
    ( (This)->lpVtbl -> DownloadUrl(This,bstrUrl,pCallback,ppStream) ) 


#define IXcpControlHost2_GetCustomAppDomain(This,ppAppDomain)	\
    ( (This)->lpVtbl -> GetCustomAppDomain(This,ppAppDomain) ) 

#define IXcpControlHost2_GetControlVersion(This,puMajorVersion,puMinorVersion)	\
    ( (This)->lpVtbl -> GetControlVersion(This,puMajorVersion,puMinorVersion) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpControlHost2_INTERFACE_DEFINED__ */


#ifndef __IXcpControlHost3_INTERFACE_DEFINED__
#define __IXcpControlHost3_INTERFACE_DEFINED__

/* interface IXcpControlHost3 */
/* [unique][helpstring][nonextensible][uuid][object] */ 

typedef /* [public] */ 
enum __MIDL_IXcpControlHost3_0001
    {	XcpHost_GetComAutomationObjectFlag_Get	= 0x1,
	XcpHost_GetComAutomationObjectFlag_Create	= 0x2
    } 	XcpHost_GetComAutomationObjectFlags;


EXTERN_C const IID IID_IXcpControlHost3;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("9fb2ce5f-06ff-4058-befa-ddfab596b3d5")
    IXcpControlHost3 : public IXcpControlHost2
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE GetDefaultThemeXaml( 
            /* [in] */ BSTR assemblyName,
            /* [retval][out] */ LPBSTR pbstrXaml) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetDefaultPortableUserInterfaceFontInfo( 
            /* [retval][out] */ LPBSTR pbstrCompositeFont) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GetComAutomationObject( 
            /* [in] */ BSTR bstrProgId,
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ IDispatch **ppDisp) = 0;
        
    };
    
#else 	/* C style interface */

    typedef struct IXcpControlHost3Vtbl
    {
        BEGIN_INTERFACE
        
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IXcpControlHost3 * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            __RPC__deref_out  void **ppvObject);
        
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IXcpControlHost3 * This);
        
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IXcpControlHost3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *GetHostOptions )( 
            IXcpControlHost3 * This,
            /* [retval][out] */ DWORD *pdwOptions);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyLoaded )( 
            IXcpControlHost3 * This);
        
        HRESULT ( STDMETHODCALLTYPE *NotifyError )( 
            IXcpControlHost3 * This,
            /* [in] */ BSTR bstrError,
            /* [in] */ BSTR bstrSource,
            /* [in] */ long nLine,
            /* [in] */ long nColumn);
        
        HRESULT ( STDMETHODCALLTYPE *InvokeHandler )( 
            IXcpControlHost3 * This,
            /* [in] */ BSTR bstrName,
            /* [in] */ VARIANT varArg1,
            /* [in] */ VARIANT varArg2,
            /* [retval][out] */ VARIANT *pvarResult);
        
        HRESULT ( STDMETHODCALLTYPE *GetBaseUrl )( 
            IXcpControlHost3 * This,
            /* [retval][out] */ BSTR *pbstrUrl);
        
        HRESULT ( STDMETHODCALLTYPE *GetNamedSource )( 
            IXcpControlHost3 * This,
            /* [in] */ BSTR bstrSourceName,
            /* [retval][out] */ BSTR *pbstrSource);
        
        HRESULT ( STDMETHODCALLTYPE *DownloadUrl )( 
            IXcpControlHost3 * This,
            /* [in] */ BSTR bstrUrl,
            /* [in] */ IXcpControlDownloadCallback *pCallback,
            /* [retval][out] */ IStream **ppStream);
        
        HRESULT ( STDMETHODCALLTYPE *GetCustomAppDomain )( 
            IXcpControlHost3 * This,
            /* [retval][out] */ IUnknown **ppAppDomain);
        
        HRESULT ( STDMETHODCALLTYPE *GetControlVersion )( 
            IXcpControlHost3 * This,
            /* [out] */ UINT *puMajorVersion,
            /* [out] */ UINT *puMinorVersion);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultThemeXaml )( 
            IXcpControlHost3 * This,
            /* [in] */ BSTR assemblyName,
            /* [retval][out] */ LPBSTR pbstrXaml);
        
        HRESULT ( STDMETHODCALLTYPE *GetDefaultPortableUserInterfaceFontInfo )( 
            IXcpControlHost3 * This,
            /* [retval][out] */ LPBSTR pbstrCompositeFont);
        
        HRESULT ( STDMETHODCALLTYPE *GetComAutomationObject )( 
            IXcpControlHost3 * This,
            /* [in] */ BSTR bstrProgId,
            /* [in] */ DWORD dwFlags,
            /* [retval][out] */ IDispatch **ppDisp);
        
        END_INTERFACE
    } IXcpControlHost3Vtbl;

    interface IXcpControlHost3
    {
        CONST_VTBL struct IXcpControlHost3Vtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IXcpControlHost3_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IXcpControlHost3_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IXcpControlHost3_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IXcpControlHost3_GetHostOptions(This,pdwOptions)	\
    ( (This)->lpVtbl -> GetHostOptions(This,pdwOptions) ) 

#define IXcpControlHost3_NotifyLoaded(This)	\
    ( (This)->lpVtbl -> NotifyLoaded(This) ) 

#define IXcpControlHost3_NotifyError(This,bstrError,bstrSource,nLine,nColumn)	\
    ( (This)->lpVtbl -> NotifyError(This,bstrError,bstrSource,nLine,nColumn) ) 

#define IXcpControlHost3_InvokeHandler(This,bstrName,varArg1,varArg2,pvarResult)	\
    ( (This)->lpVtbl -> InvokeHandler(This,bstrName,varArg1,varArg2,pvarResult) ) 

#define IXcpControlHost3_GetBaseUrl(This,pbstrUrl)	\
    ( (This)->lpVtbl -> GetBaseUrl(This,pbstrUrl) ) 

#define IXcpControlHost3_GetNamedSource(This,bstrSourceName,pbstrSource)	\
    ( (This)->lpVtbl -> GetNamedSource(This,bstrSourceName,pbstrSource) ) 

#define IXcpControlHost3_DownloadUrl(This,bstrUrl,pCallback,ppStream)	\
    ( (This)->lpVtbl -> DownloadUrl(This,bstrUrl,pCallback,ppStream) ) 


#define IXcpControlHost3_GetCustomAppDomain(This,ppAppDomain)	\
    ( (This)->lpVtbl -> GetCustomAppDomain(This,ppAppDomain) ) 

#define IXcpControlHost3_GetControlVersion(This,puMajorVersion,puMinorVersion)	\
    ( (This)->lpVtbl -> GetControlVersion(This,puMajorVersion,puMinorVersion) ) 


#define IXcpControlHost3_GetDefaultThemeXaml(This,assemblyName,pbstrXaml)	\
    ( (This)->lpVtbl -> GetDefaultThemeXaml(This,assemblyName,pbstrXaml) ) 

#define IXcpControlHost3_GetDefaultPortableUserInterfaceFontInfo(This,pbstrCompositeFont)	\
    ( (This)->lpVtbl -> GetDefaultPortableUserInterfaceFontInfo(This,pbstrCompositeFont) ) 

#define IXcpControlHost3_GetComAutomationObject(This,bstrProgId,dwFlags,ppDisp)	\
    ( (This)->lpVtbl -> GetComAutomationObject(This,bstrProgId,dwFlags,ppDisp) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IXcpControlHost3_INTERFACE_DEFINED__ */



#ifndef __XcpControlLib_LIBRARY_DEFINED__
#define __XcpControlLib_LIBRARY_DEFINED__

/* library XcpControlLib */
/* [control][helpstring][version][uuid] */ 


EXTERN_C const IID LIBID_XcpControlLib;

EXTERN_C const CLSID CLSID_XcpControl;

#ifdef __cplusplus

class DECLSPEC_UUID("DFEAF541-F3E1-4c24-ACAC-99C30715084A")
XcpControl;
#endif
#endif /* __XcpControlLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

unsigned long             __RPC_USER  VARIANT_UserSize(     unsigned long *, unsigned long            , VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserMarshal(  unsigned long *, unsigned char *, VARIANT * ); 
unsigned char * __RPC_USER  VARIANT_UserUnmarshal(unsigned long *, unsigned char *, VARIANT * ); 
void                      __RPC_USER  VARIANT_UserFree(     unsigned long *, VARIANT * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


