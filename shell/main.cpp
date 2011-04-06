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

#include <windows.h>
#include <conio.h>

#include <tbb/tbbmalloc_proxy.h>
#include <tbb/task_scheduler_observer.h>

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#endif

#include "bootstrapper.h"

#include <modules/bluefish/bluefish.h>

#include <modules/decklink/decklink.h>
#include <modules/flash/flash.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/image/image.h>

#include <common/env.h>
#include <common/exception/win32_exception.h>
#include <common/exception/exceptions.h>
#include <common/log/log.h>
#include <common/utility/assert.h>

#include <mixer/gpu/ogl_device.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>

#include <boost/foreach.hpp>

#include <Glee.h>

using namespace caspar;
using namespace caspar::core;
using namespace caspar::protocol;

#include <atlbase.h>

// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

class win32_handler_tbb_installer : public tbb::task_scheduler_observer
{
public:
	win32_handler_tbb_installer()	{observe(true);}
	void on_scheduler_entry(bool is_worker) 
	{
		//CASPAR_LOG(debug) << L"Started TBB Worker Thread.";
		win32_exception::install_handler();
	}
	void on_scheduler_exit()
	{
		//CASPAR_LOG(debug) << L"Stopped TBB Worker Thread.";
	}
};

void setup_console_window()
{	
	auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
	DrawMenuBar(GetConsoleWindow());

	auto coord = GetLargestConsoleWindowSize(hOut);
	coord.X /= 2;

	SetConsoleScreenBufferSize(hOut, coord);

	SMALL_RECT DisplayArea = {0, 0, 0, 0};
	DisplayArea.Right = coord.X-1;
	DisplayArea.Bottom = coord.Y-1;
	SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
		
	std::wstringstream str;
	str << "CasparCG Server " << env::version();
	SetConsoleTitle(str.str().c_str());
}

void print_version()
{
	CASPAR_LOG(info) << L"Copyright (c) 2010 Sveriges Television AB, www.casparcg.com, <info@casparcg.com>";
	CASPAR_LOG(info) << L"Starting CasparCG Video and Graphics Playout Server " << env::version();
	CASPAR_LOG(info) << L"Flash " << get_flash_version();
	CASPAR_LOG(info) << L"Flash-Template-Host " << get_cg_version();
	CASPAR_LOG(info) << L"FreeImage " << get_image_version();
	
	std::wstring decklink_devices;
	BOOST_FOREACH(auto& device, get_decklink_device_list())
		decklink_devices += L"\t" + device;
	CASPAR_LOG(info) << L"Decklink " << get_decklink_version() << (decklink_devices.empty() ? L"" : L"\n\tDevices:\n" + decklink_devices);
	
	std::wstring bluefish_devices;
	BOOST_FOREACH(auto& device, get_bluefish_device_list())
		bluefish_devices += L"\t" + device;
	CASPAR_LOG(info) << L"Bluefish " << get_bluefish_version() << (bluefish_devices.empty() ? L"" : L"\n\tDevices:\n" + bluefish_devices);

	CASPAR_LOG(info) << L"FFMPEG-avcodec "  << get_avcodec_version();
	CASPAR_LOG(info) << L"FFMPEG-swscale "  << get_avformat_version();
	CASPAR_LOG(info) << L"FFMPEG-avformat " << get_swscale_version();
	CASPAR_LOG(info) << L"OpenGL " << ogl_device::create()->invoke([]{return reinterpret_cast<const char*>(glGetString(GL_VERSION));})
					 << L" "	   << ogl_device::create()->invoke([]{return reinterpret_cast<const char*>(glGetString(GL_VENDOR));});

	HKEY hkey; 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t p_name_str[1024];
		wchar_t csd_ver_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(p_name_str);

		if(RegQueryValueEx(hkey, TEXT("ProductName"), NULL, &dwType, (PBYTE)&p_name_str, &dwSize) == ERROR_SUCCESS &&
		   RegQueryValueEx(hkey, TEXT("CSDVersion"), NULL, &dwType, (PBYTE)&csd_ver_str, &dwSize) == ERROR_SUCCESS)		
		{
			CASPAR_LOG(info) << p_name_str << L" " << csd_ver_str << L"." << L"\n";
		}
		else
			CASPAR_LOG(warning) << "Could not read OS info." << L"\n";
		 
		RegCloseKey(hkey);
	}
}
 
int main(int argc, wchar_t* argv[])
{		
	#ifdef _DEBUG
		_CrtSetDbgFlag ( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF );
		_CrtSetReportMode( _CRT_ERROR, _CRTDBG_MODE_DEBUG );
		_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_DEBUG );
		_CrtSetReportMode( _CRT_ASSERT, _CRTDBG_MODE_DEBUG );

		if(env::properties().get("configuration.debugging.remote", false))
			MessageBox(nullptr, TEXT("Now is the time to connect for remote debugging..."), TEXT("Debug"), MB_OK | MB_TOPMOST);
	#endif
		
	// Increase time precision
	timeBeginPeriod(1);
	win32_handler_tbb_installer win32_handler_tbb_installer;
	win32_exception::install_handler();

	try 
	{
		env::initialize("caspar.config");
		
		setup_console_window();							 
		log::add_file_sink(env::log_folder());

		print_version();
				
		bootstrapper caspar_device;
				
		auto dummy = std::make_shared<IO::ConsoleClientInfo>();
		amcp::AMCPProtocolStrategy amcp(caspar_device.get_channels());
		bool is_running = true;
		while(is_running)
		{
			std::wstring wcmd;
			std::getline(std::wcin, wcmd); // TODO: It's blocking...
			is_running = wcmd != L"exit" && wcmd != L"q";
			if(wcmd.substr(0, 2) == L"12")
			{
				wcmd = L"LOADBG 1-1 A LOOP AUTOPLAY\r\n";
				amcp.Parse(wcmd.c_str(), wcmd.length(), dummy);
				wcmd = L"LOADBG 1-2 DV LOOP AUTOPLAY\r\n";
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
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" SLIDE 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"2")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" PUSH 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"3")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" MIX 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"4")
				wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" WIPE 100 LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"5")
				wcmd = L"LOADBG 1-2 " + wcmd.substr(1, wcmd.length()-1) + L" LOOP AUTOPLAY";
			else if(wcmd.substr(0, 1) == L"6")
				wcmd = L"CG 1-2 ADD 1 BBTELEFONARE 1";
			else if(wcmd.substr(0, 1) == L"7")
				wcmd = L"LOAD 1-1 720p2500";
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
		std::wcout << L"Press Any Key To Exit\n";
		_getwch();
	}	

	timeEndPeriod(1);

	CASPAR_LOG(info) << "Successfully shutdown CasparCG Server";
	
	return 0;
}