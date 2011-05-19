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
#include "resource.h"

#include "server.h"

// tbbmalloc_proxy: 
// Replace the standard memory allocation routines in Microsoft* C/C++ RTL 
// (malloc/free, global new/delete, etc.) with the TBB memory allocator. 
#include <tbb/tbbmalloc_proxy.h>

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#define NOMINMAX

#include <windows.h>
#include <atlbase.h>
#include <conio.h>

#include <core/mixer/gpu/ogl_device.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>

#include <modules/bluefish/bluefish.h>
#include <modules/decklink/decklink.h>
#include <modules/flash/flash.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/image/image.h>

#include <common/env.h>
#include <common/exception/win32_exception.h>
#include <common/exception/exceptions.h>
#include <common/log/log.h>
#include <common/os/windows/current_version.h>
#include <common/os/windows/system_info.h>
#include <common/utility/assert.h>

#include <tbb/task_scheduler_observer.h>
#include <tbb/task_scheduler_init.h>

#include <boost/foreach.hpp>

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

void setup_console_window()
{	 
	auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Disable close button in console to avoid shutdown without cleanup.
	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
	DrawMenuBar(GetConsoleWindow());

	// Configure console size and position.
	auto coord = GetLargestConsoleWindowSize(hOut);
	coord.X /= 2;

	SetConsoleScreenBufferSize(hOut, coord);

	SMALL_RECT DisplayArea = {0, 0, 0, 0};
	DisplayArea.Right = coord.X-1;
	DisplayArea.Bottom = coord.Y-1;
	SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
		
	// Set console title.
	std::wstringstream str;
	str << "CasparCG Server " << caspar::env::version();
	SetConsoleTitle(str.str().c_str());
}

void print_info()
{
	CASPAR_LOG(info) << L"Copyright (c) 2010 Sveriges Television AB, www.casparcg.com, <info@casparcg.com>";
	CASPAR_LOG(info) << L"Starting CasparCG Video and Graphics Playout Server " << caspar::env::version();
	CASPAR_LOG(info) << L"on " << caspar::get_win_product_name() << L" " << caspar::get_win_sp_version();
	CASPAR_LOG(info) << caspar::get_cpu_info();
	CASPAR_LOG(info) << caspar::get_system_product_name();
	CASPAR_LOG(info) << L"Flash " << caspar::get_flash_version();
	CASPAR_LOG(info) << L"Flash-Template-Host " << caspar::get_cg_version();
	CASPAR_LOG(info) << L"FreeImage " << caspar::get_image_version();
	
	CASPAR_LOG(info) << L"Decklink " << caspar::get_decklink_version();
	BOOST_FOREACH(auto& device, caspar::get_decklink_device_list())
		CASPAR_LOG(info) << device;
		
	CASPAR_LOG(info) << L"Bluefish " << caspar::get_bluefish_version();
	BOOST_FOREACH(auto& device, caspar::get_bluefish_device_list())
		CASPAR_LOG(info) << device;

	CASPAR_LOG(info) << L"FFMPEG-avcodec "  << caspar::get_avcodec_version();
	CASPAR_LOG(info) << L"FFMPEG-avformat " << caspar::get_avformat_version();
	CASPAR_LOG(info) << L"FFMPEG-avfilter " << caspar::get_avfilter_version();
	CASPAR_LOG(info) << L"FFMPEG-avutil " << caspar::get_avutil_version();
	CASPAR_LOG(info) << L"FFMPEG-swscale "  << caspar::get_swscale_version();
	CASPAR_LOG(info) << L"OpenGL " << caspar::core::ogl_device::get_version() << "\n\n";
}
 
int main(int argc, wchar_t* argv[])
{	
	static_assert(sizeof(void*) == 4, "64-bit code generation is not supported.");
	
	CASPAR_LOG(info) << L"THIS IS AN UNSTABLE BUILD";

	// Set debug mode.
	#ifdef _DEBUG
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
		_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
		_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );
	#endif

	// Increase process priotity.
	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	// Install structured exception handler.
	caspar::win32_exception::install_handler();
			
	// Increase time precision. This will increase accuracy of function like Sleep(1) from 10 ms to 1 ms.
	struct inc_prec
	{
		inc_prec(){timeBeginPeriod(1);}
		~inc_prec(){timeEndPeriod(1);}
	} inc_prec;	

	// Install unstructured exception handlers into all tbb threads.
	struct tbb_thread_installer : public tbb::task_scheduler_observer
	{
		tbb_thread_installer(){observe(true);}
		void on_scheduler_entry(bool is_worker){caspar::win32_exception::install_handler();}
	} tbb_thread_installer;
	
	try 
	{
		// Configure environment properties from configuration.
		caspar::env::configure("caspar.config");

	#ifdef _DEBUG
		if(caspar::env::properties().get("configuration.debugging.remote", false))
			MessageBox(nullptr, TEXT("Now is the time to connect for remote debugging..."), TEXT("Debug"), MB_OK | MB_TOPMOST);
	#endif	 

		// Start logging to file.
		caspar::log::add_file_sink(caspar::env::log_folder());
		
		// Setup console window.
		setup_console_window();

		// Print environment information.
		print_info();
				
		// Create server object which initializes channels, protocols and controllers.
		caspar::server caspar_server;
				
		// Create a amcp parser for console commands.
		caspar::protocol::amcp::AMCPProtocolStrategy amcp(caspar_server.get_channels());

		// Create a dummy client which prints amcp responses to console.
		auto dummy = std::make_shared<caspar::IO::ConsoleClientInfo>();

		bool is_running = true;
		while(is_running)
		{
			std::wstring wcmd;
			std::getline(std::wcin, wcmd); // TODO: It's blocking...

			is_running = wcmd != L"exit" && wcmd != L"q";
			if(wcmd.substr(0, 2) == L"12")
			{
				wcmd = L"LOADBG 1-1 A LOOP \r\nPLAY 1-1\r\n";
				amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
				wcmd = L"LOADBG 1-2 DV LOOP AUTOPLAY\r\nnPLAY 1-1\r\n";
				amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
				wcmd = L"MIXER 1-1 VIDEO FIX_RECT 0.0 0.0 0.5 0.5\r\n";
				amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
				wcmd = L"MIXER 1-2 VIDEO FIX_RECT 0.5 0.0 0.5 0.5\r\n";
				amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
			}
			else if(wcmd.substr(0, 2) == L"10")
				wcmd = L"MIXER 1-1 VIDEO CLIP_RECT 0.4 0.4 0.5 0.5";
			if(wcmd.substr(0, 2) == L"11")
				wcmd = L"MIXER 1-1 VIDEO FIX_RECT 0.4 0.4 0.5 0.5";
			else if(wcmd.substr(0, 1) == L"1")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" SLIDE 100 LOOP \r\nPLAY 1-1";
			else if(wcmd.substr(0, 1) == L"2")
				wcmd = L"MIXER 1-0 VIDEO IS_KEY 1";
			else if(wcmd.substr(0, 1) == L"3")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" MIX 100 LOOP \r\nPLAY 1-1";
			else if(wcmd.substr(0, 1) == L"4")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" WIPE 100 LOOP \r\nPLAY 1-1";
			else if(wcmd.substr(0, 1) == L"5")
				wcmd = L"LOADBG 1-2 " + wcmd.substr(1, wcmd.length()-1) + L" LOOP \r\nPLAY 1-2";
			else if(wcmd.substr(0, 1) == L"6")
				wcmd = L"CG 1-2 ADD 1 BBTELEFONARE 1";
			else if(wcmd.substr(0, 1) == L"7")
				wcmd = L"CG 1-2 ADD 1 " + wcmd.substr(1, wcmd.length()-1) + L" 1";
			else if(wcmd.substr(0, 1) == L"8")
				wcmd = L"LOAD 1-1 #FFFFFFFF AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"9")
				wcmd = L"LOADBG 1-2 " + wcmd.substr(1, wcmd.length()-1) + L" [1.0-2.0] LOOP AUTOPLAY";

			wcmd += L"\r\n";
			amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
		}
	}
	catch(const std::exception&)
	{
		CASPAR_LOG(fatal) << "UNHANDLED EXCEPTION in main thread.";
		CASPAR_LOG_CURRENT_EXCEPTION();
	}	
	
	CASPAR_LOG(info) << "Successfully shutdown CasparCG Server.";
	Sleep(100); // CAPSAR_LOG is asynchronous. Try to get text in correct order.
	std::wcout << L"Press Any Key To Exit.\n";
	_getwch();
	return 0;
}