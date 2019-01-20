

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


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

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>

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

MIDL_DEFINE_GUID(IID, LIBID_ShockwaveFlashObjects,0xD27CDB6B,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);


MIDL_DEFINE_GUID(IID, IID_IShockwaveFlash,0xD27CDB6C,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);


MIDL_DEFINE_GUID(IID, DIID__IShockwaveFlashEvents,0xD27CDB6D,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);


MIDL_DEFINE_GUID(IID, IID_IFlashFactory,0xD27CDB70,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);


MIDL_DEFINE_GUID(IID, IID_IDispatchEx,0xA6EF9860,0xC720,0x11D0,0x93,0x37,0x00,0xA0,0xC9,0x0D,0xCA,0xA9);


MIDL_DEFINE_GUID(IID, IID_IFlashObjectInterface,0xD27CDB72,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);


MIDL_DEFINE_GUID(IID, IID_IServiceProvider,0x6D5140C1,0x7436,0x11CE,0x80,0x34,0x00,0xAA,0x00,0x60,0x09,0xFA);


MIDL_DEFINE_GUID(CLSID, CLSID_ShockwaveFlash,0xD27CDB6E,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);


MIDL_DEFINE_GUID(CLSID, CLSID_FlashObjectInterface,0xD27CDB71,0xAE6D,0x11CF,0x96,0xB8,0x44,0x45,0x53,0x54,0x00,0x00);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



