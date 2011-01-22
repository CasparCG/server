

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IXcpObject,0xEE38D0F1,0x5AE3,0x408c,0xA6,0xBF,0x84,0x10,0xE6,0x45,0xF3,0x76);


MIDL_DEFINE_GUID(IID, IID_IXcpControl,0x1FB839CC,0x116C,0x4C9B,0xAE,0x8E,0x3D,0xBB,0x64,0x96,0xE3,0x26);


MIDL_DEFINE_GUID(IID, IID_IXcpControl2,0x1c3294f9,0x891f,0x49b1,0xbb,0xae,0x49,0x2a,0x68,0xba,0x10,0xcc);


MIDL_DEFINE_GUID(IID, IID_IXcpControlDownloadCallback,0x2E340632,0x5D5A,0x427b,0xAC,0x31,0x30,0x3F,0x6E,0x32,0xB9,0xE8);


MIDL_DEFINE_GUID(IID, IID_IXcpControlHost,0x1B36028E,0xB491,0x4bb2,0x85,0x84,0x8A,0x9E,0x0A,0x67,0x7D,0x6E);


MIDL_DEFINE_GUID(IID, IID_IXcpControlHost2,0xfb3ed7c4,0x5797,0x4b44,0x86,0x95,0x0c,0x51,0x2e,0xa2,0x7d,0x8f);


MIDL_DEFINE_GUID(IID, IID_IXcpControlHost3,0x9fb2ce5f,0x06ff,0x4058,0xbe,0xfa,0xdd,0xfa,0xb5,0x96,0xb3,0xd5);


MIDL_DEFINE_GUID(IID, LIBID_XcpControlLib,0x283C8576,0x0726,0x4DBC,0x96,0x09,0x3F,0x85,0x51,0x62,0x00,0x9A);


MIDL_DEFINE_GUID(CLSID, CLSID_XcpControl,0xDFEAF541,0xF3E1,0x4c24,0xAC,0xAC,0x99,0xC3,0x07,0x15,0x08,0x4A);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



