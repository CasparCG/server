/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

// tbbmalloc_proxy: 
// Replace the standard memory allocation routines in Microsoft* C/C++ RTL 
// (malloc/free, global new/delete, etc.) with the TBB memory allocator. 

#include "stdafx.h"

#ifdef _DEBUG
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#else
	#include <tbb/tbbmalloc_proxy.h>
#endif

#include "resource.h"

#include "server.h"

#include <common/os/windows/windows.h>
#include <winnt.h>
#include <mmsystem.h>
#include <atlbase.h>

#include <protocol/amcp/AMCPProtocolStrategy.h>
#include <protocol/osc/server.h>

#include <modules/bluefish/bluefish.h>
#include <modules/decklink/decklink.h>
#include <modules/flash/flash.h>
#include <modules/ffmpeg/ffmpeg.h>
#include <modules/image/image.h>

#include <common/env.h>
#include <common/except.h>
#include <common/except.h>
#include <common/log.h>
#include <common/gl/gl_check.h>
#include <common/os/windows/current_version.h>
#include <common/os/windows/system_info.h>

#include <tbb/task_scheduler_init.h>
#include <tbb/task_scheduler_observer.h>

#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/foreach.hpp>
#include <boost/locale.hpp>

#include <signal.h>

using namespace caspar;
	
// NOTE: This is needed in order to make CComObject work since this is not a real ATL project.
CComModule _AtlModule;
extern __declspec(selectany) CAtlModule* _pAtlModule = &_AtlModule;

void change_icon( const HICON hNewIcon )
{
   auto hMod = ::LoadLibrary(L"Kernel32.dll"); 
   typedef DWORD(__stdcall *SCI)(HICON);
   auto pfnSetConsoleIcon = reinterpret_cast<SCI>(::GetProcAddress(hMod, "SetConsoleIcon")); 
   pfnSetConsoleIcon(hNewIcon); 
   ::FreeLibrary(hMod);
}

void setup_global_locale()
{
	boost::locale::generator gen;
	gen.categories(boost::locale::codepage_facet);

	std::locale::global(gen(""));
}

void setup_console_window()
{	 
	auto hOut = GetStdHandle(STD_OUTPUT_HANDLE);

	// Disable close button in console to avoid shutdown without cleanup.
	EnableMenuItem(GetSystemMenu(GetConsoleWindow(), FALSE), SC_CLOSE , MF_GRAYED);
	DrawMenuBar(GetConsoleWindow());
	//SetConsoleCtrlHandler(HandlerRoutine, true);

	// Configure console size and position.
	auto coord = GetLargestConsoleWindowSize(hOut);
	coord.X /= 2;

	SetConsoleScreenBufferSize(hOut, coord);

	SMALL_RECT DisplayArea = {0, 0, 0, 0};
	DisplayArea.Right = coord.X-1;
	DisplayArea.Bottom = (coord.Y-1)/2;
	SetConsoleWindowInfo(hOut, TRUE, &DisplayArea);
		 
	change_icon(::LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(101)));

	// Set console title.
	std::wstringstream str;
	str << "CasparCG Server " << env::version() << L" x64 ";
#ifdef COMPILE_RELEASE
	str << " Release";
#elif  COMPILE_PROFILE
	str << " Profile";
#elif  COMPILE_DEVELOP
	str << " Develop";
#elif  COMPILE_DEBUG
	str << " Debug";
#endif
	SetConsoleTitle(str.str().c_str());
}

void print_info()
{
	CASPAR_LOG(info) << L"################################################################################";
	CASPAR_LOG(info) << L"Copyright (c) 2010 Sveriges Television AB, www.casparcg.com, <info@casparcg.com>";
	CASPAR_LOG(info) << L"################################################################################";
	CASPAR_LOG(info) << L"Starting CasparCG Video and Graphics Playout Server " << env::version();
	CASPAR_LOG(info) << L"on " << win_product_name() << L" " << win_sp_version();
	CASPAR_LOG(info) << cpu_info();
	CASPAR_LOG(info) << system_product_name();
	
	CASPAR_LOG(info) << L"Decklink " << decklink::version();
	BOOST_FOREACH(auto device, decklink::device_list())
		CASPAR_LOG(info) << L" - " << device;	
		
	CASPAR_LOG(info) << L"Bluefish " << bluefish::version();
	BOOST_FOREACH(auto device, bluefish::device_list())
		CASPAR_LOG(info) << L" - " << device;	
	
	CASPAR_LOG(info) << L"Flash "			<< flash::version();
	CASPAR_LOG(info) << L"FreeImage "		<< image::version();
	CASPAR_LOG(info) << L"FFMPEG-avcodec "  << ffmpeg::avcodec_version();
	CASPAR_LOG(info) << L"FFMPEG-avformat " << ffmpeg::avformat_version();
	CASPAR_LOG(info) << L"FFMPEG-avfilter " << ffmpeg::avfilter_version();
	CASPAR_LOG(info) << L"FFMPEG-avutil "	<< ffmpeg::avutil_version();
	CASPAR_LOG(info) << L"FFMPEG-swscale "  << ffmpeg::swscale_version();
}

LONG WINAPI UserUnhandledExceptionFilter(EXCEPTION_POINTERS* info)
{
	try
	{
		CASPAR_LOG(fatal) << L"#######################\n UNHANDLED EXCEPTION: \n" 
			<< L"Adress:" << info->ExceptionRecord->ExceptionAddress << L"\n"
			<< L"Code:" << info->ExceptionRecord->ExceptionCode << L"\n"
			<< L"Flag:" << info->ExceptionRecord->ExceptionFlags << L"\n"
			<< L"Info:" << info->ExceptionRecord->ExceptionInformation << L"\n"
			<< L"Continuing execution. \n#######################";

		CASPAR_LOG_CALL_STACK();
	}
	catch(...){}

    return EXCEPTION_CONTINUE_EXECUTION;
}

void run()
{
	// Create server object which initializes channels, protocols and controllers.
	server caspar_server;
				
	auto server = spl::make_shared<protocol::osc::server>(5253);
	caspar_server.subscribe(server);
						
	//auto console_obs = reactive::make_observer([](const monitor::event& e)
	//{
	//	std::stringstream str;
	//	str << e;
	//	CASPAR_LOG(trace) << str.str().c_str();
	//});

	//caspar_server.subscribe(console_obs);
						
	// Create a amcp parser for console commands.
	protocol::amcp::AMCPProtocolStrategy amcp(caspar_server.channels());

	// Create a dummy client which prints amcp responses to console.
	auto console_client = std::make_shared<IO::ConsoleClientInfo>();
			
	std::wstring wcmd;
	while(true)
	{
		std::getline(std::wcin, wcmd); // TODO: It's blocking...
				
		boost::to_upper(wcmd);

		if(wcmd == L"EXIT" || wcmd == L"Q" || wcmd == L"QUIT" || wcmd == L"BYE")
			break;
				
		// This is just dummy code for testing.
		if(wcmd.substr(0, 1) == L"1")
			wcmd = L"LOADBG 1-1 " + wcmd.substr(1, wcmd.length()-1) + L" SLIDE 100 LOOP \r\nPLAY 1-1";
		else if(wcmd.substr(0, 1) == L"2")
			wcmd = L"MIXER 1-0 VIDEO IS_KEY 1";
		else if(wcmd.substr(0, 1) == L"3")
			wcmd = L"CG 1-2 ADD 1 BBTELEFONARE 1";
		else if(wcmd.substr(0, 1) == L"4")
			wcmd = L"PLAY 1-1 DV FILTER yadif=1:-1 LOOP";
		else if(wcmd.substr(0, 1) == L"5")
		{
			auto file = wcmd.substr(2, wcmd.length()-1);
			wcmd = L"PLAY 1-1 " + file + L" LOOP\r\n" 
					L"PLAY 1-2 " + file + L" LOOP\r\n" 
					L"PLAY 1-3 " + file + L" LOOP\r\n"
					L"PLAY 2-1 " + file + L" LOOP\r\n" 
					L"PLAY 2-2 " + file + L" LOOP\r\n" 
					L"PLAY 2-3 " + file + L" LOOP\r\n";
		}
		else if(wcmd.substr(0, 1) == L"7")
		{
			wcmd = L"";
			wcmd += L"CLEAR 1\r\n";
			wcmd += L"MIXER 1 CLEAR\r\n";
			wcmd += L"PLAY 1-0 GREEN\r\n";
			wcmd += L"PLAY 1-1 BLUE\r\n";
			wcmd += L"CG 1-2 ADD 1 ECS_TEST 1\r\n";
			wcmd += L"MIXER 1-2 FILL 0 -1 1 2\r\n";
		}
		else if(wcmd.substr(0, 1) == L"8")
		{
			wcmd = L"";
			wcmd += L"MIXER 1-1 FILL 0.0 0.5 1.0 1.0 500 linear DEFER\r\n";
			wcmd += L"MIXER 1-2 FILL 0.0 0.0 1.0 1.0 500 linear DEFER\r\n";
			wcmd += L"MIXER 1 COMMIT\r\n";
		}
		else if(wcmd.substr(0, 1) == L"X")
		{
			int num = 0;
			std::wstring file;
			try
			{
				num = boost::lexical_cast<int>(wcmd.substr(1, 2));
				file = wcmd.substr(4, wcmd.length()-1);
			}
			catch(...)
			{
				num = boost::lexical_cast<int>(wcmd.substr(1, 1));
				file = wcmd.substr(3, wcmd.length()-1);
			}

			int n = 0;
			int num2 = num;
			while(num2 > 0)
			{
				num2 >>= 1;
				n++;
			}

			wcmd = L"MIXER 1 GRID " + boost::lexical_cast<std::wstring>(n);

			for(int i = 1; i <= num; ++i)
				wcmd += L"\r\nPLAY 1-" + boost::lexical_cast<std::wstring>(i) + L" " + file + L" LOOP";// + L" SLIDE 100 LOOP";
		}

		wcmd += L"\r\n";
		amcp.Parse(wcmd.c_str(), static_cast<int>(wcmd.length()), console_client);
	}	
	CASPAR_LOG(info) << "Successfully shutdown CasparCG Server.";
}

void on_abort(int)
{
	CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("abort called"));
}

int main(int argc, wchar_t* argv[])
{	
	SetUnhandledExceptionFilter(UserUnhandledExceptionFilter);
	signal(SIGABRT, on_abort);

	setup_global_locale();

	std::wcout << L"Type \"q\" to close application." << std::endl;
	
	// Set debug mode.
	#ifdef _DEBUG
		HANDLE hLogFile;
		hLogFile = CreateFile(L"crt_log.txt", GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		std::shared_ptr<void> crt_log(nullptr, [](HANDLE h){::CloseHandle(h);});

		_CrtSetDbgFlag (_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
		_CrtSetReportMode(_CRT_WARN,	_CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_WARN,	hLogFile);
		_CrtSetReportMode(_CRT_ERROR,	_CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ERROR,	hLogFile);
		_CrtSetReportMode(_CRT_ASSERT,	_CRTDBG_MODE_FILE);
		_CrtSetReportFile(_CRT_ASSERT,	hLogFile);
	#endif

	// Increase process priotity.
	SetPriorityClass(GetCurrentProcess(), ABOVE_NORMAL_PRIORITY_CLASS);

	// Install structured exception handler.
	win32_exception::install_handler();
				
	// Increase time precision. This will increase accuracy of function like Sleep(1) from 10 ms to 1 ms.
	struct inc_prec
	{
		inc_prec(){timeBeginPeriod(1);}
		~inc_prec(){timeEndPeriod(1);}
	} inc_prec;	

	// Install SEH into all tbb threads.
	struct tbb_thread_installer : public tbb::task_scheduler_observer
	{
		tbb_thread_installer(){observe(true);}
		void on_scheduler_entry(bool is_worker)
		{
			//detail::SetThreadName(GetCurrentThreadId(), "tbb-worker-thread");
			win32_exception::install_handler();
		}
	} tbb_thread_installer;

	tbb::task_scheduler_init init;
	
	try 
	{
		// Configure environment properties from configuration.
		env::configure(L"casparcg.config");
				
		log::set_log_level(env::properties().get(L"configuration.log-level", L"debug"));

	#ifdef _DEBUG
		if(env::properties().get(L"configuration.debugging.remote", false))
			MessageBox(nullptr, TEXT("Now is the time to connect for remote debugging..."), TEXT("Debug"), MB_OK | MB_TOPMOST);
	#endif	 

		// Start logging to file.
		log::add_file_sink(env::log_folder());			
		std::wcout << L"Logging [info] or higher severity to " << env::log_folder() << std::endl << std::endl;
		
		// Setup console window.
		setup_console_window();

		// Print environment information.
		print_info();
		
		std::wstringstream str;
		boost::property_tree::xml_writer_settings<wchar_t> w(' ', 3);
		boost::property_tree::write_xml(str, env::properties(), w);
		CASPAR_LOG(info) << L"casparcg.config:\n-----------------------------------------\n" << str.str().c_str() << L"-----------------------------------------";
		
		run();
		
		system("pause");	
	}
	catch(boost::property_tree::file_parser_error&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(fatal) << L"Unhandled configuration error in main thread. Please check the configuration file (casparcg.config) for errors.";
		system("pause");	
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(fatal) << L"Unhandled exception in main thread. Please report this error on the CasparCG forums (www.casparcg.com/forum).";
		Sleep(1000);
		std::wcout << L"\n\nCasparCG will automatically shutdown. See the log file located at the configured log-file folder for more information.\n\n";
		Sleep(4000);
	}		
	
	return 0;
}