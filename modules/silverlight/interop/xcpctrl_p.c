

/* this ALWAYS GENERATED file contains the proxy stub code */


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

#if !defined(_M_IA64) && !defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "xcpctrl_h.h"

#define TYPE_FORMAT_STRING_SIZE   1163                              
#define PROC_FORMAT_STRING_SIZE   1237                              
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   2            

typedef struct _xcpctrl_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } xcpctrl_MIDL_TYPE_FORMAT_STRING;

typedef struct _xcpctrl_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } xcpctrl_MIDL_PROC_FORMAT_STRING;

typedef struct _xcpctrl_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } xcpctrl_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const xcpctrl_MIDL_TYPE_FORMAT_STRING xcpctrl__MIDL_TypeFormatString;
extern const xcpctrl_MIDL_PROC_FORMAT_STRING xcpctrl__MIDL_ProcFormatString;
extern const xcpctrl_MIDL_EXPR_FORMAT_STRING xcpctrl__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpObject_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpObject_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpControl_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpControl_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpControl2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpControl2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpControlDownloadCallback_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpControlDownloadCallback_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpControlHost_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpControlHost_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpControlHost2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpControlHost2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IXcpControlHost3_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IXcpControlHost3_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const xcpctrl_MIDL_PROC_FORMAT_STRING xcpctrl__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure GetBaseUrl */


	/* Procedure get_Source */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x7 ),	/* 7 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x8 ),	/* 8 */
/* 14 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 16 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 18 */	NdrFcShort( 0x1 ),	/* 1 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pbstrUrl */


	/* Parameter pstr */

/* 24 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 26 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 28 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */


	/* Return value */

/* 30 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 32 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 34 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_Source */

/* 36 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 38 */	NdrFcLong( 0x0 ),	/* 0 */
/* 42 */	NdrFcShort( 0x8 ),	/* 8 */
/* 44 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 46 */	NdrFcShort( 0x0 ),	/* 0 */
/* 48 */	NdrFcShort( 0x8 ),	/* 8 */
/* 50 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 52 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x1 ),	/* 1 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter str */

/* 60 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 62 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 64 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 66 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 68 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 70 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_IsLoaded */

/* 72 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 74 */	NdrFcLong( 0x0 ),	/* 0 */
/* 78 */	NdrFcShort( 0x9 ),	/* 9 */
/* 80 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 82 */	NdrFcShort( 0x0 ),	/* 0 */
/* 84 */	NdrFcShort( 0x22 ),	/* 34 */
/* 86 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 88 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pb */

/* 96 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 98 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 100 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 102 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 104 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 106 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_Content */

/* 108 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 110 */	NdrFcLong( 0x0 ),	/* 0 */
/* 114 */	NdrFcShort( 0xa ),	/* 10 */
/* 116 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 118 */	NdrFcShort( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x8 ),	/* 8 */
/* 122 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 124 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 126 */	NdrFcShort( 0x0 ),	/* 0 */
/* 128 */	NdrFcShort( 0x0 ),	/* 0 */
/* 130 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ppContent */

/* 132 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 134 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 136 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Return value */

/* 138 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 140 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 142 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_Settings */

/* 144 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 146 */	NdrFcLong( 0x0 ),	/* 0 */
/* 150 */	NdrFcShort( 0xb ),	/* 11 */
/* 152 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 158 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 160 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 166 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ppSettings */

/* 168 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 170 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 172 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Return value */

/* 174 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 176 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 178 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_OnLoad */

/* 180 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 182 */	NdrFcLong( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0xc ),	/* 12 */
/* 188 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 190 */	NdrFcShort( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x8 ),	/* 8 */
/* 194 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 196 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 198 */	NdrFcShort( 0x1 ),	/* 1 */
/* 200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 204 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 206 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 208 */	NdrFcShort( 0x428 ),	/* Type Offset=1064 */

	/* Return value */

/* 210 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 212 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 214 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_OnLoad */

/* 216 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 218 */	NdrFcLong( 0x0 ),	/* 0 */
/* 222 */	NdrFcShort( 0xd ),	/* 13 */
/* 224 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0x8 ),	/* 8 */
/* 230 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 232 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 236 */	NdrFcShort( 0x1 ),	/* 1 */
/* 238 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 240 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 242 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 244 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Return value */

/* 246 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 248 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 250 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_OnError */

/* 252 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 254 */	NdrFcLong( 0x0 ),	/* 0 */
/* 258 */	NdrFcShort( 0xe ),	/* 14 */
/* 260 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0x8 ),	/* 8 */
/* 266 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 268 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 270 */	NdrFcShort( 0x1 ),	/* 1 */
/* 272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 274 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 276 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 278 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 280 */	NdrFcShort( 0x428 ),	/* Type Offset=1064 */

	/* Return value */

/* 282 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 284 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 286 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_OnError */

/* 288 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 290 */	NdrFcLong( 0x0 ),	/* 0 */
/* 294 */	NdrFcShort( 0xf ),	/* 15 */
/* 296 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 298 */	NdrFcShort( 0x0 ),	/* 0 */
/* 300 */	NdrFcShort( 0x8 ),	/* 8 */
/* 302 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 304 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 306 */	NdrFcShort( 0x0 ),	/* 0 */
/* 308 */	NdrFcShort( 0x1 ),	/* 1 */
/* 310 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 312 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 314 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 316 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Return value */

/* 318 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 320 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 322 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CreateObject */

/* 324 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 326 */	NdrFcLong( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x10 ),	/* 16 */
/* 332 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0x8 ),	/* 8 */
/* 338 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 340 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 342 */	NdrFcShort( 0x0 ),	/* 0 */
/* 344 */	NdrFcShort( 0x1 ),	/* 1 */
/* 346 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter id */

/* 348 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 350 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 352 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter ppDisp */

/* 354 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 356 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 358 */	NdrFcShort( 0x444 ),	/* Type Offset=1092 */

	/* Return value */

/* 360 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 362 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 364 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure IsVersionSupported */

/* 366 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 368 */	NdrFcLong( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x11 ),	/* 17 */
/* 374 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 376 */	NdrFcShort( 0x0 ),	/* 0 */
/* 378 */	NdrFcShort( 0x22 ),	/* 34 */
/* 380 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 382 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 384 */	NdrFcShort( 0x0 ),	/* 0 */
/* 386 */	NdrFcShort( 0x1 ),	/* 1 */
/* 388 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter v */

/* 390 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 392 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 394 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter pb */

/* 396 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 398 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 400 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 402 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 404 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 406 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_InitParams */

/* 408 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 410 */	NdrFcLong( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x12 ),	/* 18 */
/* 416 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 418 */	NdrFcShort( 0x0 ),	/* 0 */
/* 420 */	NdrFcShort( 0x8 ),	/* 8 */
/* 422 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 424 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 426 */	NdrFcShort( 0x1 ),	/* 1 */
/* 428 */	NdrFcShort( 0x0 ),	/* 0 */
/* 430 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter initparams */

/* 432 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 434 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 436 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 438 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 440 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 442 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_InitParams */

/* 444 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 446 */	NdrFcLong( 0x0 ),	/* 0 */
/* 450 */	NdrFcShort( 0x13 ),	/* 19 */
/* 452 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 456 */	NdrFcShort( 0x8 ),	/* 8 */
/* 458 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 460 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 462 */	NdrFcShort( 0x0 ),	/* 0 */
/* 464 */	NdrFcShort( 0x1 ),	/* 1 */
/* 466 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter initparams */

/* 468 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 470 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 472 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 474 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 476 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 478 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LoadRuntime */

/* 480 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 482 */	NdrFcLong( 0x0 ),	/* 0 */
/* 486 */	NdrFcShort( 0x14 ),	/* 20 */
/* 488 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 492 */	NdrFcShort( 0x8 ),	/* 8 */
/* 494 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 496 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 500 */	NdrFcShort( 0x0 ),	/* 0 */
/* 502 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 504 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 506 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 508 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_SplashScreenSource */

/* 510 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 512 */	NdrFcLong( 0x0 ),	/* 0 */
/* 516 */	NdrFcShort( 0x15 ),	/* 21 */
/* 518 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	NdrFcShort( 0x8 ),	/* 8 */
/* 524 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 526 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 528 */	NdrFcShort( 0x1 ),	/* 1 */
/* 530 */	NdrFcShort( 0x0 ),	/* 0 */
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pstr */

/* 534 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 536 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 538 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 540 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 542 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 544 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_SplashScreenSource */

/* 546 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 548 */	NdrFcLong( 0x0 ),	/* 0 */
/* 552 */	NdrFcShort( 0x16 ),	/* 22 */
/* 554 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0x8 ),	/* 8 */
/* 560 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 562 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x1 ),	/* 1 */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter str */

/* 570 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 572 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 574 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Return value */

/* 576 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 578 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 580 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_OnSourceDownloadComplete */

/* 582 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 584 */	NdrFcLong( 0x0 ),	/* 0 */
/* 588 */	NdrFcShort( 0x17 ),	/* 23 */
/* 590 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 592 */	NdrFcShort( 0x0 ),	/* 0 */
/* 594 */	NdrFcShort( 0x8 ),	/* 8 */
/* 596 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 598 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 600 */	NdrFcShort( 0x1 ),	/* 1 */
/* 602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 604 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 606 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 608 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 610 */	NdrFcShort( 0x428 ),	/* Type Offset=1064 */

	/* Return value */

/* 612 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 614 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 616 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_OnSourceDownloadComplete */

/* 618 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 620 */	NdrFcLong( 0x0 ),	/* 0 */
/* 624 */	NdrFcShort( 0x18 ),	/* 24 */
/* 626 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 630 */	NdrFcShort( 0x8 ),	/* 8 */
/* 632 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 634 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 638 */	NdrFcShort( 0x1 ),	/* 1 */
/* 640 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 642 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 644 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 646 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Return value */

/* 648 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 650 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_OnSourceDownloadProgressChanged */

/* 654 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 656 */	NdrFcLong( 0x0 ),	/* 0 */
/* 660 */	NdrFcShort( 0x19 ),	/* 25 */
/* 662 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 664 */	NdrFcShort( 0x0 ),	/* 0 */
/* 666 */	NdrFcShort( 0x8 ),	/* 8 */
/* 668 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 670 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 672 */	NdrFcShort( 0x1 ),	/* 1 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 678 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 680 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 682 */	NdrFcShort( 0x428 ),	/* Type Offset=1064 */

	/* Return value */

/* 684 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 686 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 688 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_OnSourceDownloadProgressChanged */

/* 690 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 692 */	NdrFcLong( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x1a ),	/* 26 */
/* 698 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 702 */	NdrFcShort( 0x8 ),	/* 8 */
/* 704 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 706 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 710 */	NdrFcShort( 0x1 ),	/* 1 */
/* 712 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pVAR */

/* 714 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 716 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 718 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Return value */

/* 720 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 722 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 724 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnUrlDownloaded */

/* 726 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 728 */	NdrFcLong( 0x0 ),	/* 0 */
/* 732 */	NdrFcShort( 0x3 ),	/* 3 */
/* 734 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 736 */	NdrFcShort( 0x8 ),	/* 8 */
/* 738 */	NdrFcShort( 0x8 ),	/* 8 */
/* 740 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 742 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hr */

/* 750 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 752 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 754 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pStream */

/* 756 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 758 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 760 */	NdrFcShort( 0x45a ),	/* Type Offset=1114 */

	/* Return value */

/* 762 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 764 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 766 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetHostOptions */

/* 768 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 770 */	NdrFcLong( 0x0 ),	/* 0 */
/* 774 */	NdrFcShort( 0x3 ),	/* 3 */
/* 776 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 778 */	NdrFcShort( 0x0 ),	/* 0 */
/* 780 */	NdrFcShort( 0x24 ),	/* 36 */
/* 782 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 784 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 788 */	NdrFcShort( 0x0 ),	/* 0 */
/* 790 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pdwOptions */

/* 792 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 794 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 798 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 800 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 802 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NotifyLoaded */

/* 804 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 806 */	NdrFcLong( 0x0 ),	/* 0 */
/* 810 */	NdrFcShort( 0x4 ),	/* 4 */
/* 812 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 814 */	NdrFcShort( 0x0 ),	/* 0 */
/* 816 */	NdrFcShort( 0x8 ),	/* 8 */
/* 818 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 820 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 824 */	NdrFcShort( 0x0 ),	/* 0 */
/* 826 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 828 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 830 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 832 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure NotifyError */

/* 834 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 836 */	NdrFcLong( 0x0 ),	/* 0 */
/* 840 */	NdrFcShort( 0x5 ),	/* 5 */
/* 842 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 844 */	NdrFcShort( 0x10 ),	/* 16 */
/* 846 */	NdrFcShort( 0x8 ),	/* 8 */
/* 848 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 850 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 852 */	NdrFcShort( 0x0 ),	/* 0 */
/* 854 */	NdrFcShort( 0x1 ),	/* 1 */
/* 856 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrError */

/* 858 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 860 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 862 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter bstrSource */

/* 864 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 866 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 868 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter nLine */

/* 870 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 872 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 874 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter nColumn */

/* 876 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 878 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 880 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 882 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 884 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure InvokeHandler */

/* 888 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 890 */	NdrFcLong( 0x0 ),	/* 0 */
/* 894 */	NdrFcShort( 0x6 ),	/* 6 */
/* 896 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 898 */	NdrFcShort( 0x0 ),	/* 0 */
/* 900 */	NdrFcShort( 0x8 ),	/* 8 */
/* 902 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 904 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 906 */	NdrFcShort( 0x1 ),	/* 1 */
/* 908 */	NdrFcShort( 0x1 ),	/* 1 */
/* 910 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrName */

/* 912 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 914 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 916 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter varArg1 */

/* 918 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 920 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 922 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter varArg2 */

/* 924 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 926 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 928 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter pvarResult */

/* 930 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 932 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 934 */	NdrFcShort( 0x428 ),	/* Type Offset=1064 */

	/* Return value */

/* 936 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 938 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 940 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetNamedSource */

/* 942 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 944 */	NdrFcLong( 0x0 ),	/* 0 */
/* 948 */	NdrFcShort( 0x8 ),	/* 8 */
/* 950 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x8 ),	/* 8 */
/* 956 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 958 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 960 */	NdrFcShort( 0x1 ),	/* 1 */
/* 962 */	NdrFcShort( 0x1 ),	/* 1 */
/* 964 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrSourceName */

/* 966 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 968 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 970 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter pbstrSource */

/* 972 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 974 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 976 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 978 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 980 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 982 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DownloadUrl */

/* 984 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 986 */	NdrFcLong( 0x0 ),	/* 0 */
/* 990 */	NdrFcShort( 0x9 ),	/* 9 */
/* 992 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 994 */	NdrFcShort( 0x0 ),	/* 0 */
/* 996 */	NdrFcShort( 0x8 ),	/* 8 */
/* 998 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1000 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1006 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrUrl */

/* 1008 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1010 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1012 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter pCallback */

/* 1014 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 1016 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1018 */	NdrFcShort( 0x470 ),	/* Type Offset=1136 */

	/* Parameter ppStream */

/* 1020 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1022 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1024 */	NdrFcShort( 0x482 ),	/* Type Offset=1154 */

	/* Return value */

/* 1026 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1028 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1030 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetCustomAppDomain */

/* 1032 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1034 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0xa ),	/* 10 */
/* 1040 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1042 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1044 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1046 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1048 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1050 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1052 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1054 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ppAppDomain */

/* 1056 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1058 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1060 */	NdrFcShort( 0x486 ),	/* Type Offset=1158 */

	/* Return value */

/* 1062 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1064 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1066 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetControlVersion */

/* 1068 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1070 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1074 */	NdrFcShort( 0xb ),	/* 11 */
/* 1076 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1078 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1080 */	NdrFcShort( 0x40 ),	/* 64 */
/* 1082 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 1084 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1086 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1088 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1090 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter puMajorVersion */

/* 1092 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1094 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1096 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter puMinorVersion */

/* 1098 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1100 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1102 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 1104 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1106 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1108 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDefaultThemeXaml */

/* 1110 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1112 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1116 */	NdrFcShort( 0xc ),	/* 12 */
/* 1118 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1122 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1124 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1126 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 1128 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1130 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1132 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter assemblyName */

/* 1134 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1136 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1138 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter pbstrXaml */

/* 1140 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1142 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1144 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 1146 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1148 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1150 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetDefaultPortableUserInterfaceFontInfo */

/* 1152 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1154 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1158 */	NdrFcShort( 0xd ),	/* 13 */
/* 1160 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1162 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1164 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1166 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1168 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1170 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1174 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter pbstrCompositeFont */

/* 1176 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1178 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1180 */	NdrFcShort( 0x20 ),	/* Type Offset=32 */

	/* Return value */

/* 1182 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1186 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GetComAutomationObject */

/* 1188 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1190 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1194 */	NdrFcShort( 0xe ),	/* 14 */
/* 1196 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 1198 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1200 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1202 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 1204 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1208 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1210 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter bstrProgId */

/* 1212 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1214 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1216 */	NdrFcShort( 0x2e ),	/* Type Offset=46 */

	/* Parameter dwFlags */

/* 1218 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 1220 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter ppDisp */

/* 1224 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1226 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1228 */	NdrFcShort( 0x3c ),	/* Type Offset=60 */

	/* Return value */

/* 1230 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1232 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1234 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const xcpctrl_MIDL_TYPE_FORMAT_STRING xcpctrl__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/*  4 */	NdrFcShort( 0x1c ),	/* Offset= 28 (32) */
/*  6 */	
			0x13, 0x0,	/* FC_OP */
/*  8 */	NdrFcShort( 0xe ),	/* Offset= 14 (22) */
/* 10 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 12 */	NdrFcShort( 0x2 ),	/* 2 */
/* 14 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 16 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 18 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 20 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 22 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 24 */	NdrFcShort( 0x8 ),	/* 8 */
/* 26 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (10) */
/* 28 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 30 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 32 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0x4 ),	/* 4 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0xffde ),	/* Offset= -34 (6) */
/* 42 */	
			0x12, 0x0,	/* FC_UP */
/* 44 */	NdrFcShort( 0xffea ),	/* Offset= -22 (22) */
/* 46 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 48 */	NdrFcShort( 0x0 ),	/* 0 */
/* 50 */	NdrFcShort( 0x4 ),	/* 4 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (42) */
/* 56 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 58 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 60 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 62 */	NdrFcShort( 0x2 ),	/* Offset= 2 (64) */
/* 64 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 66 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 70 */	NdrFcShort( 0x0 ),	/* 0 */
/* 72 */	NdrFcShort( 0x0 ),	/* 0 */
/* 74 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 76 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 78 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 80 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 82 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 84 */	NdrFcShort( 0x3d4 ),	/* Offset= 980 (1064) */
/* 86 */	
			0x13, 0x0,	/* FC_OP */
/* 88 */	NdrFcShort( 0x3bc ),	/* Offset= 956 (1044) */
/* 90 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 92 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 94 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 96 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 98 */	NdrFcShort( 0x2 ),	/* Offset= 2 (100) */
/* 100 */	NdrFcShort( 0x10 ),	/* 16 */
/* 102 */	NdrFcShort( 0x2f ),	/* 47 */
/* 104 */	NdrFcLong( 0x14 ),	/* 20 */
/* 108 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 110 */	NdrFcLong( 0x3 ),	/* 3 */
/* 114 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 116 */	NdrFcLong( 0x11 ),	/* 17 */
/* 120 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 122 */	NdrFcLong( 0x2 ),	/* 2 */
/* 126 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 128 */	NdrFcLong( 0x4 ),	/* 4 */
/* 132 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 134 */	NdrFcLong( 0x5 ),	/* 5 */
/* 138 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 140 */	NdrFcLong( 0xb ),	/* 11 */
/* 144 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 146 */	NdrFcLong( 0xa ),	/* 10 */
/* 150 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 152 */	NdrFcLong( 0x6 ),	/* 6 */
/* 156 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (388) */
/* 158 */	NdrFcLong( 0x7 ),	/* 7 */
/* 162 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 164 */	NdrFcLong( 0x8 ),	/* 8 */
/* 168 */	NdrFcShort( 0xff5e ),	/* Offset= -162 (6) */
/* 170 */	NdrFcLong( 0xd ),	/* 13 */
/* 174 */	NdrFcShort( 0xdc ),	/* Offset= 220 (394) */
/* 176 */	NdrFcLong( 0x9 ),	/* 9 */
/* 180 */	NdrFcShort( 0xff8c ),	/* Offset= -116 (64) */
/* 182 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 186 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (412) */
/* 188 */	NdrFcLong( 0x24 ),	/* 36 */
/* 192 */	NdrFcShort( 0x30a ),	/* Offset= 778 (970) */
/* 194 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 198 */	NdrFcShort( 0x304 ),	/* Offset= 772 (970) */
/* 200 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 204 */	NdrFcShort( 0x302 ),	/* Offset= 770 (974) */
/* 206 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 210 */	NdrFcShort( 0x300 ),	/* Offset= 768 (978) */
/* 212 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 216 */	NdrFcShort( 0x2fe ),	/* Offset= 766 (982) */
/* 218 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 222 */	NdrFcShort( 0x2fc ),	/* Offset= 764 (986) */
/* 224 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 228 */	NdrFcShort( 0x2fa ),	/* Offset= 762 (990) */
/* 230 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 234 */	NdrFcShort( 0x2f8 ),	/* Offset= 760 (994) */
/* 236 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 240 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (978) */
/* 242 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 246 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (982) */
/* 248 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 252 */	NdrFcShort( 0x2ea ),	/* Offset= 746 (998) */
/* 254 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 258 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (994) */
/* 260 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 264 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (1002) */
/* 266 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 270 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (1006) */
/* 272 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 276 */	NdrFcShort( 0x2de ),	/* Offset= 734 (1010) */
/* 278 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 282 */	NdrFcShort( 0x2dc ),	/* Offset= 732 (1014) */
/* 284 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 288 */	NdrFcShort( 0x2da ),	/* Offset= 730 (1018) */
/* 290 */	NdrFcLong( 0x10 ),	/* 16 */
/* 294 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 296 */	NdrFcLong( 0x12 ),	/* 18 */
/* 300 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 302 */	NdrFcLong( 0x13 ),	/* 19 */
/* 306 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 308 */	NdrFcLong( 0x15 ),	/* 21 */
/* 312 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 314 */	NdrFcLong( 0x16 ),	/* 22 */
/* 318 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 320 */	NdrFcLong( 0x17 ),	/* 23 */
/* 324 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 326 */	NdrFcLong( 0xe ),	/* 14 */
/* 330 */	NdrFcShort( 0x2b8 ),	/* Offset= 696 (1026) */
/* 332 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 336 */	NdrFcShort( 0x2bc ),	/* Offset= 700 (1036) */
/* 338 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 342 */	NdrFcShort( 0x2ba ),	/* Offset= 698 (1040) */
/* 344 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 348 */	NdrFcShort( 0x276 ),	/* Offset= 630 (978) */
/* 350 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 354 */	NdrFcShort( 0x274 ),	/* Offset= 628 (982) */
/* 356 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 360 */	NdrFcShort( 0x272 ),	/* Offset= 626 (986) */
/* 362 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 366 */	NdrFcShort( 0x268 ),	/* Offset= 616 (982) */
/* 368 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 372 */	NdrFcShort( 0x262 ),	/* Offset= 610 (982) */
/* 374 */	NdrFcLong( 0x0 ),	/* 0 */
/* 378 */	NdrFcShort( 0x0 ),	/* Offset= 0 (378) */
/* 380 */	NdrFcLong( 0x1 ),	/* 1 */
/* 384 */	NdrFcShort( 0x0 ),	/* Offset= 0 (384) */
/* 386 */	NdrFcShort( 0xffff ),	/* Offset= -1 (385) */
/* 388 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 390 */	NdrFcShort( 0x8 ),	/* 8 */
/* 392 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 394 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 396 */	NdrFcLong( 0x0 ),	/* 0 */
/* 400 */	NdrFcShort( 0x0 ),	/* 0 */
/* 402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 404 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 406 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 408 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 410 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 412 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 414 */	NdrFcShort( 0x2 ),	/* Offset= 2 (416) */
/* 416 */	
			0x13, 0x0,	/* FC_OP */
/* 418 */	NdrFcShort( 0x216 ),	/* Offset= 534 (952) */
/* 420 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 422 */	NdrFcShort( 0x18 ),	/* 24 */
/* 424 */	NdrFcShort( 0xa ),	/* 10 */
/* 426 */	NdrFcLong( 0x8 ),	/* 8 */
/* 430 */	NdrFcShort( 0x5a ),	/* Offset= 90 (520) */
/* 432 */	NdrFcLong( 0xd ),	/* 13 */
/* 436 */	NdrFcShort( 0x7e ),	/* Offset= 126 (562) */
/* 438 */	NdrFcLong( 0x9 ),	/* 9 */
/* 442 */	NdrFcShort( 0x9e ),	/* Offset= 158 (600) */
/* 444 */	NdrFcLong( 0xc ),	/* 12 */
/* 448 */	NdrFcShort( 0xc8 ),	/* Offset= 200 (648) */
/* 450 */	NdrFcLong( 0x24 ),	/* 36 */
/* 454 */	NdrFcShort( 0x124 ),	/* Offset= 292 (746) */
/* 456 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 460 */	NdrFcShort( 0x140 ),	/* Offset= 320 (780) */
/* 462 */	NdrFcLong( 0x10 ),	/* 16 */
/* 466 */	NdrFcShort( 0x15a ),	/* Offset= 346 (812) */
/* 468 */	NdrFcLong( 0x2 ),	/* 2 */
/* 472 */	NdrFcShort( 0x174 ),	/* Offset= 372 (844) */
/* 474 */	NdrFcLong( 0x3 ),	/* 3 */
/* 478 */	NdrFcShort( 0x18e ),	/* Offset= 398 (876) */
/* 480 */	NdrFcLong( 0x14 ),	/* 20 */
/* 484 */	NdrFcShort( 0x1a8 ),	/* Offset= 424 (908) */
/* 486 */	NdrFcShort( 0xffff ),	/* Offset= -1 (485) */
/* 488 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 490 */	NdrFcShort( 0x4 ),	/* 4 */
/* 492 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 494 */	NdrFcShort( 0x0 ),	/* 0 */
/* 496 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 498 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 500 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 502 */	NdrFcShort( 0x4 ),	/* 4 */
/* 504 */	NdrFcShort( 0x0 ),	/* 0 */
/* 506 */	NdrFcShort( 0x1 ),	/* 1 */
/* 508 */	NdrFcShort( 0x0 ),	/* 0 */
/* 510 */	NdrFcShort( 0x0 ),	/* 0 */
/* 512 */	0x13, 0x0,	/* FC_OP */
/* 514 */	NdrFcShort( 0xfe14 ),	/* Offset= -492 (22) */
/* 516 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 518 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 520 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 522 */	NdrFcShort( 0x8 ),	/* 8 */
/* 524 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 526 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 528 */	NdrFcShort( 0x4 ),	/* 4 */
/* 530 */	NdrFcShort( 0x4 ),	/* 4 */
/* 532 */	0x11, 0x0,	/* FC_RP */
/* 534 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (488) */
/* 536 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 538 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 540 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 542 */	NdrFcShort( 0x0 ),	/* 0 */
/* 544 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 548 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 550 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 554 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 556 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 558 */	NdrFcShort( 0xff5c ),	/* Offset= -164 (394) */
/* 560 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 562 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 564 */	NdrFcShort( 0x8 ),	/* 8 */
/* 566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0x6 ),	/* Offset= 6 (574) */
/* 570 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 572 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 574 */	
			0x11, 0x0,	/* FC_RP */
/* 576 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (540) */
/* 578 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 580 */	NdrFcShort( 0x0 ),	/* 0 */
/* 582 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 584 */	NdrFcShort( 0x0 ),	/* 0 */
/* 586 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 588 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 592 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 594 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 596 */	NdrFcShort( 0xfdec ),	/* Offset= -532 (64) */
/* 598 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 600 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 602 */	NdrFcShort( 0x8 ),	/* 8 */
/* 604 */	NdrFcShort( 0x0 ),	/* 0 */
/* 606 */	NdrFcShort( 0x6 ),	/* Offset= 6 (612) */
/* 608 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 610 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 612 */	
			0x11, 0x0,	/* FC_RP */
/* 614 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (578) */
/* 616 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 618 */	NdrFcShort( 0x4 ),	/* 4 */
/* 620 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 622 */	NdrFcShort( 0x0 ),	/* 0 */
/* 624 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 626 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 628 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 630 */	NdrFcShort( 0x4 ),	/* 4 */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	NdrFcShort( 0x1 ),	/* 1 */
/* 636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 638 */	NdrFcShort( 0x0 ),	/* 0 */
/* 640 */	0x13, 0x0,	/* FC_OP */
/* 642 */	NdrFcShort( 0x192 ),	/* Offset= 402 (1044) */
/* 644 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 646 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 648 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 650 */	NdrFcShort( 0x8 ),	/* 8 */
/* 652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 654 */	NdrFcShort( 0x6 ),	/* Offset= 6 (660) */
/* 656 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 658 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 660 */	
			0x11, 0x0,	/* FC_RP */
/* 662 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (616) */
/* 664 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 666 */	NdrFcLong( 0x2f ),	/* 47 */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 676 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 678 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 680 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 682 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 684 */	NdrFcShort( 0x1 ),	/* 1 */
/* 686 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 688 */	NdrFcShort( 0x4 ),	/* 4 */
/* 690 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 692 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 694 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 696 */	NdrFcShort( 0x10 ),	/* 16 */
/* 698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 700 */	NdrFcShort( 0xa ),	/* Offset= 10 (710) */
/* 702 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 704 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 706 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (664) */
/* 708 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 710 */	
			0x13, 0x0,	/* FC_OP */
/* 712 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (682) */
/* 714 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 716 */	NdrFcShort( 0x4 ),	/* 4 */
/* 718 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 722 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 724 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 726 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 728 */	NdrFcShort( 0x4 ),	/* 4 */
/* 730 */	NdrFcShort( 0x0 ),	/* 0 */
/* 732 */	NdrFcShort( 0x1 ),	/* 1 */
/* 734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 736 */	NdrFcShort( 0x0 ),	/* 0 */
/* 738 */	0x13, 0x0,	/* FC_OP */
/* 740 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (694) */
/* 742 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 744 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 746 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 748 */	NdrFcShort( 0x8 ),	/* 8 */
/* 750 */	NdrFcShort( 0x0 ),	/* 0 */
/* 752 */	NdrFcShort( 0x6 ),	/* Offset= 6 (758) */
/* 754 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 756 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 758 */	
			0x11, 0x0,	/* FC_RP */
/* 760 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (714) */
/* 762 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 764 */	NdrFcShort( 0x8 ),	/* 8 */
/* 766 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 768 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 770 */	NdrFcShort( 0x10 ),	/* 16 */
/* 772 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 774 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 776 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (762) */
			0x5b,		/* FC_END */
/* 780 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 782 */	NdrFcShort( 0x18 ),	/* 24 */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 786 */	NdrFcShort( 0xa ),	/* Offset= 10 (796) */
/* 788 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 790 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 792 */	NdrFcShort( 0xffe8 ),	/* Offset= -24 (768) */
/* 794 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 796 */	
			0x11, 0x0,	/* FC_RP */
/* 798 */	NdrFcShort( 0xfefe ),	/* Offset= -258 (540) */
/* 800 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 802 */	NdrFcShort( 0x1 ),	/* 1 */
/* 804 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 806 */	NdrFcShort( 0x0 ),	/* 0 */
/* 808 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 810 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 812 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 814 */	NdrFcShort( 0x8 ),	/* 8 */
/* 816 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 818 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 820 */	NdrFcShort( 0x4 ),	/* 4 */
/* 822 */	NdrFcShort( 0x4 ),	/* 4 */
/* 824 */	0x13, 0x0,	/* FC_OP */
/* 826 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (800) */
/* 828 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 830 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 832 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 834 */	NdrFcShort( 0x2 ),	/* 2 */
/* 836 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 838 */	NdrFcShort( 0x0 ),	/* 0 */
/* 840 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 842 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 844 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 846 */	NdrFcShort( 0x8 ),	/* 8 */
/* 848 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 850 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 852 */	NdrFcShort( 0x4 ),	/* 4 */
/* 854 */	NdrFcShort( 0x4 ),	/* 4 */
/* 856 */	0x13, 0x0,	/* FC_OP */
/* 858 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (832) */
/* 860 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 862 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 864 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 866 */	NdrFcShort( 0x4 ),	/* 4 */
/* 868 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 872 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 874 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 876 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 878 */	NdrFcShort( 0x8 ),	/* 8 */
/* 880 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 882 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 884 */	NdrFcShort( 0x4 ),	/* 4 */
/* 886 */	NdrFcShort( 0x4 ),	/* 4 */
/* 888 */	0x13, 0x0,	/* FC_OP */
/* 890 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (864) */
/* 892 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 894 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 896 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 898 */	NdrFcShort( 0x8 ),	/* 8 */
/* 900 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 902 */	NdrFcShort( 0x0 ),	/* 0 */
/* 904 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 906 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 908 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 910 */	NdrFcShort( 0x8 ),	/* 8 */
/* 912 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 914 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 916 */	NdrFcShort( 0x4 ),	/* 4 */
/* 918 */	NdrFcShort( 0x4 ),	/* 4 */
/* 920 */	0x13, 0x0,	/* FC_OP */
/* 922 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (896) */
/* 924 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 926 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 928 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 930 */	NdrFcShort( 0x8 ),	/* 8 */
/* 932 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 934 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 936 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 938 */	NdrFcShort( 0x8 ),	/* 8 */
/* 940 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 942 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 944 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 946 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 948 */	NdrFcShort( 0xffec ),	/* Offset= -20 (928) */
/* 950 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 952 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 954 */	NdrFcShort( 0x28 ),	/* 40 */
/* 956 */	NdrFcShort( 0xffec ),	/* Offset= -20 (936) */
/* 958 */	NdrFcShort( 0x0 ),	/* Offset= 0 (958) */
/* 960 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 962 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 964 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 966 */	NdrFcShort( 0xfdde ),	/* Offset= -546 (420) */
/* 968 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 970 */	
			0x13, 0x0,	/* FC_OP */
/* 972 */	NdrFcShort( 0xfeea ),	/* Offset= -278 (694) */
/* 974 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 976 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 978 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 980 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 982 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 984 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 986 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 988 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 990 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 992 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 994 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 996 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 998 */	
			0x13, 0x0,	/* FC_OP */
/* 1000 */	NdrFcShort( 0xfd9c ),	/* Offset= -612 (388) */
/* 1002 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1004 */	NdrFcShort( 0xfc1a ),	/* Offset= -998 (6) */
/* 1006 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1008 */	NdrFcShort( 0xfd9a ),	/* Offset= -614 (394) */
/* 1010 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1012 */	NdrFcShort( 0xfc4c ),	/* Offset= -948 (64) */
/* 1014 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1016 */	NdrFcShort( 0xfda4 ),	/* Offset= -604 (412) */
/* 1018 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1020 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1022) */
/* 1022 */	
			0x13, 0x0,	/* FC_OP */
/* 1024 */	NdrFcShort( 0x14 ),	/* Offset= 20 (1044) */
/* 1026 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 1028 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1030 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 1032 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 1034 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1036 */	
			0x13, 0x0,	/* FC_OP */
/* 1038 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (1026) */
/* 1040 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1042 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 1044 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 1046 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1048 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1050 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1050) */
/* 1052 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1054 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1056 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1058 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1060 */	NdrFcShort( 0xfc36 ),	/* Offset= -970 (90) */
/* 1062 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1064 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1066 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1068 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1070 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1072 */	NdrFcShort( 0xfc26 ),	/* Offset= -986 (86) */
/* 1074 */	
			0x11, 0x0,	/* FC_RP */
/* 1076 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1082) */
/* 1078 */	
			0x12, 0x0,	/* FC_UP */
/* 1080 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (1044) */
/* 1082 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1084 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1086 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1088 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1090 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (1078) */
/* 1092 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1094 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1096) */
/* 1096 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1098 */	NdrFcLong( 0xee38d0f1 ),	/* -298266383 */
/* 1102 */	NdrFcShort( 0x5ae3 ),	/* 23267 */
/* 1104 */	NdrFcShort( 0x408c ),	/* 16524 */
/* 1106 */	0xa6,		/* 166 */
			0xbf,		/* 191 */
/* 1108 */	0x84,		/* 132 */
			0x10,		/* 16 */
/* 1110 */	0xe6,		/* 230 */
			0x45,		/* 69 */
/* 1112 */	0xf3,		/* 243 */
			0x76,		/* 118 */
/* 1114 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1116 */	NdrFcLong( 0xc ),	/* 12 */
/* 1120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1124 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 1126 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1128 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 1130 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 1132 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1134 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 1136 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1138 */	NdrFcLong( 0x2e340632 ),	/* 775161394 */
/* 1142 */	NdrFcShort( 0x5d5a ),	/* 23898 */
/* 1144 */	NdrFcShort( 0x427b ),	/* 17019 */
/* 1146 */	0xac,		/* 172 */
			0x31,		/* 49 */
/* 1148 */	0x30,		/* 48 */
			0x3f,		/* 63 */
/* 1150 */	0x6e,		/* 110 */
			0x32,		/* 50 */
/* 1152 */	0xb9,		/* 185 */
			0xe8,		/* 232 */
/* 1154 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1156 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (1114) */
/* 1158 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1160 */	NdrFcShort( 0xfd02 ),	/* Offset= -766 (394) */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            },
            {
            VARIANT_UserSize
            ,VARIANT_UserMarshal
            ,VARIANT_UserUnmarshal
            ,VARIANT_UserFree
            }

        };



/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IXcpObject, ver. 0.0,
   GUID={0xEE38D0F1,0x5AE3,0x408c,{0xA6,0xBF,0x84,0x10,0xE6,0x45,0xF3,0x76}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpObject_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpObject_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpObject_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpObject_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpObject_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IXcpObjectProxyVtbl = 
{
    0,
    &IID_IXcpObject,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */
};


static const PRPC_STUB_FUNCTION IXcpObject_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION
};

CInterfaceStubVtbl _IXcpObjectStubVtbl =
{
    &IID_IXcpObject,
    &IXcpObject_ServerInfo,
    7,
    &IXcpObject_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IXcpControl, ver. 0.0,
   GUID={0x1FB839CC,0x116C,0x4C9B,{0xAE,0x8E,0x3D,0xBB,0x64,0x96,0xE3,0x26}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpControl_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    36,
    72,
    108,
    144,
    180,
    216,
    252,
    288,
    324,
    366,
    408,
    444
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpControl_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControl_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpControl_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControl_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(20) _IXcpControlProxyVtbl = 
{
    &IXcpControl_ProxyInfo,
    &IID_IXcpControl,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_Source */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_Source */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_IsLoaded */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_Content */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_Settings */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_OnLoad */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_OnLoad */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_OnError */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_OnError */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::CreateObject */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::IsVersionSupported */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_InitParams */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_InitParams */
};


static const PRPC_STUB_FUNCTION IXcpControl_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IXcpControlStubVtbl =
{
    &IID_IXcpControl,
    &IXcpControl_ServerInfo,
    20,
    &IXcpControl_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IXcpControl2, ver. 0.0,
   GUID={0x1c3294f9,0x891f,0x49b1,{0xbb,0xae,0x49,0x2a,0x68,0xba,0x10,0xcc}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpControl2_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    36,
    72,
    108,
    144,
    180,
    216,
    252,
    288,
    324,
    366,
    408,
    444,
    480,
    510,
    546,
    582,
    618,
    654,
    690
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpControl2_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControl2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpControl2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControl2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(27) _IXcpControl2ProxyVtbl = 
{
    &IXcpControl2_ProxyInfo,
    &IID_IXcpControl2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_Source */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_Source */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_IsLoaded */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_Content */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_Settings */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_OnLoad */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_OnLoad */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_OnError */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_OnError */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::CreateObject */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::IsVersionSupported */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::get_InitParams */ ,
    (void *) (INT_PTR) -1 /* IXcpControl::put_InitParams */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::LoadRuntime */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::get_SplashScreenSource */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::put_SplashScreenSource */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::get_OnSourceDownloadComplete */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::put_OnSourceDownloadComplete */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::get_OnSourceDownloadProgressChanged */ ,
    (void *) (INT_PTR) -1 /* IXcpControl2::put_OnSourceDownloadProgressChanged */
};


static const PRPC_STUB_FUNCTION IXcpControl2_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IXcpControl2StubVtbl =
{
    &IID_IXcpControl2,
    &IXcpControl2_ServerInfo,
    27,
    &IXcpControl2_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IXcpControlDownloadCallback, ver. 0.0,
   GUID={0x2E340632,0x5D5A,0x427b,{0xAC,0x31,0x30,0x3F,0x6E,0x32,0xB9,0xE8}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpControlDownloadCallback_FormatStringOffsetTable[] =
    {
    726
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpControlDownloadCallback_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlDownloadCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpControlDownloadCallback_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlDownloadCallback_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IXcpControlDownloadCallbackProxyVtbl = 
{
    &IXcpControlDownloadCallback_ProxyInfo,
    &IID_IXcpControlDownloadCallback,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IXcpControlDownloadCallback::OnUrlDownloaded */
};

const CInterfaceStubVtbl _IXcpControlDownloadCallbackStubVtbl =
{
    &IID_IXcpControlDownloadCallback,
    &IXcpControlDownloadCallback_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IXcpControlHost, ver. 0.0,
   GUID={0x1B36028E,0xB491,0x4bb2,{0x85,0x84,0x8A,0x9E,0x0A,0x67,0x7D,0x6E}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpControlHost_FormatStringOffsetTable[] =
    {
    768,
    804,
    834,
    888,
    0,
    942,
    984
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpControlHost_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlHost_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpControlHost_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlHost_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IXcpControlHostProxyVtbl = 
{
    &IXcpControlHost_ProxyInfo,
    &IID_IXcpControlHost,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetHostOptions */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::NotifyLoaded */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::NotifyError */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::InvokeHandler */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetBaseUrl */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetNamedSource */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::DownloadUrl */
};

const CInterfaceStubVtbl _IXcpControlHostStubVtbl =
{
    &IID_IXcpControlHost,
    &IXcpControlHost_ServerInfo,
    10,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IXcpControlHost2, ver. 0.0,
   GUID={0xfb3ed7c4,0x5797,0x4b44,{0x86,0x95,0x0c,0x51,0x2e,0xa2,0x7d,0x8f}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpControlHost2_FormatStringOffsetTable[] =
    {
    768,
    804,
    834,
    888,
    0,
    942,
    984,
    1032,
    1068
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpControlHost2_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlHost2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpControlHost2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlHost2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(12) _IXcpControlHost2ProxyVtbl = 
{
    &IXcpControlHost2_ProxyInfo,
    &IID_IXcpControlHost2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetHostOptions */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::NotifyLoaded */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::NotifyError */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::InvokeHandler */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetBaseUrl */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetNamedSource */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::DownloadUrl */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost2::GetCustomAppDomain */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost2::GetControlVersion */
};

const CInterfaceStubVtbl _IXcpControlHost2StubVtbl =
{
    &IID_IXcpControlHost2,
    &IXcpControlHost2_ServerInfo,
    12,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IXcpControlHost3, ver. 0.0,
   GUID={0x9fb2ce5f,0x06ff,0x4058,{0xbe,0xfa,0xdd,0xfa,0xb5,0x96,0xb3,0xd5}} */

#pragma code_seg(".orpc")
static const unsigned short IXcpControlHost3_FormatStringOffsetTable[] =
    {
    768,
    804,
    834,
    888,
    0,
    942,
    984,
    1032,
    1068,
    1110,
    1152,
    1188
    };

static const MIDL_STUBLESS_PROXY_INFO IXcpControlHost3_ProxyInfo =
    {
    &Object_StubDesc,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlHost3_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IXcpControlHost3_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    xcpctrl__MIDL_ProcFormatString.Format,
    &IXcpControlHost3_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(15) _IXcpControlHost3ProxyVtbl = 
{
    &IXcpControlHost3_ProxyInfo,
    &IID_IXcpControlHost3,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetHostOptions */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::NotifyLoaded */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::NotifyError */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::InvokeHandler */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetBaseUrl */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::GetNamedSource */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost::DownloadUrl */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost2::GetCustomAppDomain */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost2::GetControlVersion */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost3::GetDefaultThemeXaml */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost3::GetDefaultPortableUserInterfaceFontInfo */ ,
    (void *) (INT_PTR) -1 /* IXcpControlHost3::GetComAutomationObject */
};

const CInterfaceStubVtbl _IXcpControlHost3StubVtbl =
{
    &IID_IXcpControlHost3,
    &IXcpControlHost3_ServerInfo,
    15,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    xcpctrl__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x700022b, /* MIDL Version 7.0.555 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _xcpctrl_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IXcpControlDownloadCallbackProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IXcpControlHost3ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IXcpControlHostProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IXcpControlHost2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IXcpControlProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IXcpObjectProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IXcpControl2ProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _xcpctrl_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IXcpControlDownloadCallbackStubVtbl,
    ( CInterfaceStubVtbl *) &_IXcpControlHost3StubVtbl,
    ( CInterfaceStubVtbl *) &_IXcpControlHostStubVtbl,
    ( CInterfaceStubVtbl *) &_IXcpControlHost2StubVtbl,
    ( CInterfaceStubVtbl *) &_IXcpControlStubVtbl,
    ( CInterfaceStubVtbl *) &_IXcpObjectStubVtbl,
    ( CInterfaceStubVtbl *) &_IXcpControl2StubVtbl,
    0
};

PCInterfaceName const _xcpctrl_InterfaceNamesList[] = 
{
    "IXcpControlDownloadCallback",
    "IXcpControlHost3",
    "IXcpControlHost",
    "IXcpControlHost2",
    "IXcpControl",
    "IXcpObject",
    "IXcpControl2",
    0
};

const IID *  const _xcpctrl_BaseIIDList[] = 
{
    0,
    0,
    0,
    0,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    0
};


#define _xcpctrl_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _xcpctrl, pIID, n)

int __stdcall _xcpctrl_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _xcpctrl, 7, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _xcpctrl, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _xcpctrl, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _xcpctrl, 7, *pIndex )
    
}

const ExtendedProxyFileInfo xcpctrl_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _xcpctrl_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _xcpctrl_StubVtblList,
    (const PCInterfaceName * ) & _xcpctrl_InterfaceNamesList,
    (const IID ** ) & _xcpctrl_BaseIIDList,
    & _xcpctrl_IID_Lookup, 
    7,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#pragma optimize("", on )
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64)*/

