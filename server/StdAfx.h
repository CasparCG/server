// stdafx.h : include file for standard system include files,
//  or project specific include files that are used frequently, but
//      are changed infrequently
//

#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _DEBUG
#include <crtdbg.h>
#endif

#include <winsock2.h>
#include <tchar.h>
#include <sstream>
#include <memory>

#ifndef TEMPLATEHOST_VERSION
#define TEMPLATEHOST_VERSION 1700
#endif


#ifndef _UNICODE
typedef std::ostringstream tstringstream;
typedef std::string tstring;
#else
typedef std::wostringstream tstringstream;
typedef std::wstring tstring;
#endif

#include <assert.h>

#include "utils\Logger.h"
#define LOG	caspar::utils::Logger::GetInstance().GetStream(caspar::utils::LogLevel::Release)


	#include <atlbase.h>
	#include <atlapp.h>

	extern WTL::CAppModule _Module;

	#include <atlcom.h>
	#include <atlhost.h>

	extern LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

namespace caspar
{
	class Application;
	Application* GetApplication();
};
//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
