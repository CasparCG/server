#pragma once

#undef _UNICODE
#define _UNICODE
#undef UNICODE
#define UNICODE

#undef NOMINMAX
#define NOMINMAX

#undef NOSERVICE
#define NOSERVICE
#undef NOMCX
#define NOMCX

#ifdef _MSC_VER
#include <SDKDDKVer.h>
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT _WIN32_WINNT_WIN7
#endif

#undef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
