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

#include <Windows.h>

#include <tbb/tbbmalloc_proxy.h>
#include <tbb/task_scheduler_observer.h>

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#include <conio.h>

#include <core/config.h>
#include <core/server.h>
#include <core/protocol/amcp/AMCPProtocolStrategy.h>
#include <common/exception/win32_exception.h>
#include <common/exception/exceptions.h>
#include <common/log/log.h>

using namespace caspar;
using namespace caspar::core;

class win32_handler_tbb_installer : public tbb::task_scheduler_observer
{
public:
	win32_handler_tbb_installer()	{observe(true);}
	void on_scheduler_entry(bool is_worker) 
	{
		CASPAR_LOG(debug) << L"Started TBB Worker Thread.";
		win32_exception::install_handler();
	}
};
 
int main(int argc, wchar_t* argv[])
{
	std::wstringstream str;
	str << "CasparCG " << CASPAR_VERSION_STR << " " << CASPAR_VERSION_TAG;
	SetConsoleTitle(str.str().c_str());

	CASPAR_LOG(info) << L"Starting CasparCG Video Playout Server Ver: " << CASPAR_VERSION_STR << " Tag: " << CASPAR_VERSION_TAG << std::endl;
	CASPAR_LOG(info) << L"Copyright (c) 2010 Sveriges Television AB <info@casparcg.com>\n\n" << std::endl;

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

	log::add_file_sink(server::log_folder());
	
	CASPAR_LOG(debug) << "Started Main Thread";

	win32_handler_tbb_installer win32_handler_tbb_installer;
	win32_exception::install_handler();
				
	try 
	{
		server caspar_device;
				
		auto dummy = std::make_shared<IO::DummyClientInfo>();
		amcp::AMCPProtocolStrategy amcp(caspar_device.get_channels());
		bool is_running = true;
		while(is_running)
		{
			std::wstring wcmd;
			std::getline(std::wcin, wcmd); // TODO: It's blocking...
			is_running = wcmd != L"exit" && wcmd != L"q";
			if(wcmd.substr(0, 1) == L"1")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" SLIDE 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"2")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" PUSH 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"3")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" MIX 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"4")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" WIPE 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"5")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"6")
				wcmd = L"CG 1-2 ADD 1 BBTELEFONARE 1";
			else if(wcmd.substr(0, 1) == L"7")
				wcmd = L"LOAD 1-1 720p2500";
			else if(wcmd.substr(0, 1) == L"8")
				wcmd = L"LOAD 1-1 #FFFFFFFF AUTOPLAY";

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