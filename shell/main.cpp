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

#include <tbb/task_scheduler_init.h>
#include <tbb/task_scheduler_observer.h>

#if defined _DEBUG && defined _MSC_VER
	#define _CRTDBG_MAP_ALLOC
	#include <stdlib.h>
	#include <crtdbg.h>
#else
	#include <tbb/tbbmalloc_proxy.h>
#endif

#include "server.h"
#include "platform_specific.h"
#include "included_modules.h"

#include <protocol/util/strategy_adapters.h>
#include <protocol/amcp/AMCPProtocolStrategy.h>

#include <common/env.h>
#include <common/except.h>
#include <common/log.h>
#include <common/gl/gl_check.h>
#include <common/os/system_info.h>
#include <common/os/general_protection_fault.h>

#include <core/system_info_provider.h>

#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/locale.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/thread.hpp>
#include <boost/thread/future.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <future>

#include <csignal>

using namespace caspar;
	
void setup_global_locale()
{
	boost::locale::generator gen;
	gen.categories(boost::locale::codepage_facet);

	std::locale::global(gen(""));
}

void print_info()
{
	CASPAR_LOG(info) << L"############################################################################";
	CASPAR_LOG(info) << L"CasparCG Server is distributed by the Swedish Broadcasting Corporation (SVT)";
	CASPAR_LOG(info) << L"under the GNU General Public License GPLv3 or higher.";
	CASPAR_LOG(info) << L"Please see LICENSE.TXT for details.";
	CASPAR_LOG(info) << L"http://www.casparcg.com/";
	CASPAR_LOG(info) << L"############################################################################";
	CASPAR_LOG(info) << L"Starting CasparCG Video and Graphics Playout Server " << env::version();
	CASPAR_LOG(info) << L"on " << os_description();
	CASPAR_LOG(info) << cpu_info();
	CASPAR_LOG(info) << system_product_name();
}

void print_child(const std::wstring& indent, const std::wstring& elem, const boost::property_tree::wptree& tree)
{
	auto data = tree.data();

	if (data.empty())
		CASPAR_LOG(info) << indent << elem;
	else
		CASPAR_LOG(info) << indent << elem << L" " << tree.data();

	for (auto& child : tree)
		print_child(indent + L"    ", child.first, child.second);
}

void print_system_info(const spl::shared_ptr<core::system_info_provider_repository>& repo)
{
	boost::property_tree::wptree info;
	repo->fill_information(info);

	for (auto& elem : info.get_child(L"system"))
		print_child(L"", elem.first, elem.second);
}

void do_run(server& caspar_server, std::promise<bool>& shutdown_server_now)
{
	// Create a dummy client which prints amcp responses to console.
	auto console_client = spl::make_shared<IO::ConsoleClientInfo>();
	
	// Create a amcp parser for console commands.
	auto amcp = spl::make_shared<caspar::IO::delimiter_based_chunking_strategy_factory<wchar_t>>(
			L"\r\n",
			spl::make_shared<caspar::IO::legacy_strategy_adapter_factory>(
					spl::make_shared<protocol::amcp::AMCPProtocolStrategy>(
							caspar_server.channels(),
							caspar_server.get_thumbnail_generator(),
							caspar_server.get_media_info_repo(),
							caspar_server.get_system_info_provider_repo(),
							caspar_server.get_cg_registry(),
							shutdown_server_now)))->create(console_client);

	std::wstring wcmd;
	while(true)
	{
		std::getline(std::wcin, wcmd); // TODO: It's blocking...
				
		//boost::to_upper(wcmd);

		if(boost::iequals(wcmd, L"EXIT") || boost::iequals(wcmd, L"Q") || boost::iequals(wcmd, L"QUIT") || boost::iequals(wcmd, L"BYE"))
		{
			shutdown_server_now.set_value(true);	//true to wait for keypress
			break;
		}

		try
		{
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
		}
		catch (...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			continue;
		}

		wcmd += L"\r\n";
		amcp->parse(wcmd);
	}
};

bool run()
{
	std::promise<bool> shutdown_server_now;
	std::future<bool> shutdown_server = shutdown_server_now.get_future();

	print_info();

	// Create server object which initializes channels, protocols and controllers.
	server caspar_server(shutdown_server_now);
	
	// Print environment information.
	print_system_info(caspar_server.get_system_info_provider_repo());

	std::wstringstream str;
	boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);
	boost::property_tree::write_xml(str, env::properties(), w);
	CASPAR_LOG(info) << L"casparcg.config:\n-----------------------------------------\n" << str.str() << L"-----------------------------------------";
	
	caspar_server.start();

	//auto console_obs = reactive::make_observer([](const monitor::event& e)
	//{
	//	std::stringstream str;
	//	str << e;
	//	CASPAR_LOG(trace) << str.str().c_str();
	//});

	//caspar_server.subscribe(console_obs);


	// Use separate thread for the blocking console input, will be terminated 
	// anyway when the main thread terminates.
	boost::thread stdin_thread(std::bind(do_run, std::ref(caspar_server), std::ref(shutdown_server_now)));	//compiler didn't like lambda here...
	stdin_thread.detach();
	return shutdown_server.get();
}

void on_abort(int)
{
	CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("abort called"));
}

int main(int argc, char** argv)
{
	if (intercept_command_line_args(argc, argv))
		return 0;

	int return_code = 0;
	setup_prerequisites();
	//std::signal(SIGABRT, on_abort);

	setup_global_locale();

	std::wcout << L"Type \"q\" to close application." << std::endl;
	
	// Set debug mode.
	auto debugging_environment = setup_debugging_environment();

	// Increase process priotity.
	increase_process_priority();

	// Install general protection fault handler.
	ensure_gpf_handler_installed_for_thread("main thread");

	// Install GPF handler into all tbb threads.
	struct tbb_thread_installer : public tbb::task_scheduler_observer
	{
		tbb_thread_installer(){observe(true);}
		void on_scheduler_entry(bool is_worker)
		{
			ensure_gpf_handler_installed_for_thread("tbb-worker-thread");
		}
	} tbb_thread_installer;

	tbb::task_scheduler_init init;
	
	try 
	{
		// Configure environment properties from configuration.
		env::configure(L"casparcg.config");

		log::set_log_level(env::properties().get(L"configuration.log-level", L"debug"));

		if (env::properties().get(L"configuration.debugging.remote", false))
			wait_for_remote_debugging();

		// Start logging to file.
		log::add_file_sink(env::log_folder());			
		std::wcout << L"Logging [info] or higher severity to " << env::log_folder() << std::endl << std::endl;
		
		// Setup console window.
		setup_console_window();

		return_code = run() ? 5 : 0;
		
		boost::this_thread::sleep_for(boost::chrono::milliseconds(500));
		CASPAR_LOG(info) << "Successfully shutdown CasparCG Server.";
	}
	catch(boost::property_tree::file_parser_error&)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(fatal) << L"Unhandled configuration error in main thread. Please check the configuration file (casparcg.config) for errors.";
		wait_for_keypress();
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(fatal) << L"Unhandled exception in main thread. Please report this error on the CasparCG forums (www.casparcg.com/forum).";
		boost::this_thread::sleep_for(boost::chrono::milliseconds(1000));
		std::wcout << L"\n\nCasparCG will automatically shutdown. See the log file located at the configured log-file folder for more information.\n\n";
		boost::this_thread::sleep_for(boost::chrono::milliseconds(4000));
	}
	
	return return_code;
}
