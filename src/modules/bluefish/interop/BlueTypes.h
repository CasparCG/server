////////////////////////////////////////////////////////////////////////////
// File:        BlueTypes.h
//
// Description: Declaration for Bluefish types
//
// (C) Copyright 2017 by Bluefish Technologies Pty Ltd. All Rights Reserved.
////////////////////////////////////////////////////////////////////////////


#ifndef HG_BLUE_TYPES_HG
#define HG_BLUE_TYPES_HG


#define BLUE_TRUE	1
#define BLUE_FALSE	0

typedef int                BERR;
typedef int                BErr;
typedef long long          BGERROR;
typedef void               BLUE_VOID;
typedef int                BLUE_BOOL;
typedef unsigned char      BLUE_U8;
typedef unsigned short     BLUE_U16;
typedef unsigned int       BLUE_U32;
typedef unsigned long long BLUE_U64;
typedef char               BLUE_S8;
typedef short              BLUE_S16;
typedef int                BLUE_S32;
typedef long long          BLUE_S64;
typedef BLUE_U8            BLUE_UINT8;
typedef BLUE_U16           BLUE_UINT16;
typedef BLUE_U32           BLUE_UINT32;
typedef BLUE_U64           BLUE_UINT64;
typedef BLUE_S8            BLUE_INT8;
typedef BLUE_S16           BLUE_INT16;
typedef BLUE_S32           BLUE_INT32;
typedef BLUE_S64           BLUE_INT64;

// TODO: maybe make a macro to do all this ifndef def etuff.
#ifdef __linux__

#ifndef ULONG
    #define ULONG BLUE_U32
#endif
#ifndef BOOLEAN 
    #define BOOLEAN bool
#endif
#ifndef BOOL
    #define BOOL bool
#endif
#ifndef LPVOID
    #define LPVOID void*
#endif
#ifndef LPDWORD
    #define LPDWORD BLUE_U32*
#endif    
#ifndef LPOVERLAPPED
    #define LPOVERLAPPED void*
#endif

typedef void*              PVOID;
typedef void*              PVOID64;
typedef BLUE_U64           LARGE_INTEGER;
typedef BLUE_U32           DWORD;
#endif

#endif //#define HG_BLUE_TYPES_HG
