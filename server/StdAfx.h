/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
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
