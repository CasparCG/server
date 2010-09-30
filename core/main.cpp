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

#include "StdAfx.h"

#include <tbb/tbbmalloc_proxy.h>
#include <tbb/task_scheduler_observer.h>

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#include <conio.h>

#include "server.h"
#include "protocol/amcp/AMCPProtocolStrategy.h"

class win32_handler_tbb_installer : public tbb::task_scheduler_observer
{
public:
	win32_handler_tbb_installer()	{observe(true);}
	void on_scheduler_entry(bool is_worker) {caspar::win32_exception::install_handler();} 
};
 
int _tmain(int argc, _TCHAR* argv[])
{
	std::wstringstream str;
	str << "CasparCG " << CASPAR_VERSION_STR << " " << CASPAR_VERSION_TAG;
	SetConsoleTitle(str.str().c_str());

	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
    DrawMenuBar(GetConsoleWindow());
	MoveWindow(GetConsoleWindow(), 800, 0, 800, 1000, true);

#ifdef _DEBUG
	_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );
	_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
	_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );

	MessageBox(nullptr, TEXT("Now is the time to connect for remote debugging..."), TEXT("Debug"), MB_OK | MB_TOPMOST);
#endif

	caspar::log::add_file_sink(caspar::server::log_folder());

	win32_handler_tbb_installer win32_handler_tbb_installer;
	caspar::win32_exception::install_handler();
		
	std::wcout << L"Starting CasparCG Video Playout Server Ver: " << CASPAR_VERSION_STR << " tag: " << CASPAR_VERSION_TAG << std::endl;
	std::wcout << L"Copyright (c) 2010 Sveriges Television AB <info@casparcg.com>\n\n" << std::endl;
		
	CASPAR_LOG(debug) << "Started Main Thread";
	try 
	{
		caspar::server caspar_device;
				
		auto dummy = std::make_shared<caspar::IO::DummyClientInfo>();
		caspar::amcp::AMCPProtocolStrategy amcp(caspar_device.get_channels());
		bool is_running = true;
		while(is_running)
		{
			std::wstring wcmd;
			std::getline(std::wcin, wcmd); // TODO: It's blocking...
			is_running = wcmd != L"exit" && wcmd != L"q";
			if(wcmd == L"1")
				wcmd = L"LOADBG 1-1 DV SLIDE 50 LOOP AUTOPLAY";
			else if(wcmd == L"2")
				wcmd = L"CG 1-2 ADD 1 BBTELEFONARE 1";

			wcmd += L"\r\n";
			amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
		}
	}
	catch(const std::exception&)
	{
		CASPAR_LOG(fatal) << "UNHANDLED EXCEPTION in main thread.";
		CASPAR_LOG_CURRENT_EXCEPTION();
		std::wcout << L"Press Any Key To Exit";
		_getwch();
	}	
	CASPAR_LOG(debug) << "Ended Main Thread";

	return 0;
}
