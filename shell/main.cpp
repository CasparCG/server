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
	// Reenable when tbb gets official support for vc14
	//#include <tbb/tbbmalloc_proxy.h>
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

#include <boost/property_tree/detail/file_parser_error.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/locale.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <boost/stacktrace.hpp>
#include <boost/filesystem.hpp>

#include <atomic>
#include <future>
#include <set>

#include <csignal>
#include <clocale>

namespace caspar {

void setup_global_locale()
{
	boost::locale::generator gen;
	gen.categories(boost::locale::codepage_facet);

	std::locale::global(gen(""));

	// sscanf is used in for example FFmpeg where we want decimals to be parsed as .
	std::setlocale(LC_ALL, "C");
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
}

void do_run(
		std::weak_ptr<caspar::IO::protocol_strategy<wchar_t>> amcp,
		std::promise<bool>& shutdown_server_now,
		std::atomic<bool>& should_wait_for_keypress)
{
	std::wstring wcmd;
	while(true)
	{
		if (!std::getline(std::wcin, wcmd))		// TODO: It's blocking...
			wcmd = L"EXIT";						// EOF, handle as EXIT

		if(boost::iequals(wcmd, L"EXIT") || boost::iequals(wcmd, L"Q") || boost::iequals(wcmd, L"QUIT") || boost::iequals(wcmd, L"BYE"))
		{
			CASPAR_LOG(info) << L"Received message from Console: " << wcmd << L"\\r\\n";
			should_wait_for_keypress = true;
			shutdown_server_now.set_value(false);	//false to not restart
			break;
		}

		wcmd += L"\r\n";
		auto strong = amcp.lock();
		if (strong)
			strong->parse(wcmd);
		else
			break;
	}
};

bool run(const std::wstring& config_file_name, std::atomic<bool>& should_wait_for_keypress)
{
	std::promise<bool> shutdown_server_now;
	std::future<bool> shutdown_server = shutdown_server_now.get_future();

	print_info();

	// Create server object which initializes channels, protocols and controllers.
	std::unique_ptr<server> caspar_server(new server(shutdown_server_now));

	// For example CEF resets the global locale, so this is to reset it back to "our" preference.
	setup_global_locale();

	std::wstringstream str;
	boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);
	boost::property_tree::write_xml(str, env::properties(), w);
	CASPAR_LOG(info) << config_file_name << L":\n-----------------------------------------\n" << str.str() << L"-----------------------------------------";

	{
		caspar_server->start();
	}

	// Create a dummy client which prints amcp responses to console.
	auto console_client = spl::make_shared<IO::ConsoleClientInfo>();

	// Create a amcp parser for console commands.
	std::shared_ptr<IO::protocol_strategy<wchar_t>> amcp = spl::make_shared<caspar::IO::delimiter_based_chunking_strategy_factory<wchar_t>>(
			L"\r\n",
			spl::make_shared<caspar::IO::legacy_strategy_adapter_factory>(
					spl::make_shared<protocol::amcp::AMCPProtocolStrategy>(
							L"Console",
							caspar_server->get_amcp_command_repository())))->create(console_client);
	std::weak_ptr<IO::protocol_strategy<wchar_t>> weak_amcp = amcp;

	// Use separate thread for the blocking console input, will be terminated
	// anyway when the main thread terminates.
	std::thread stdin_thread(std::bind(do_run, weak_amcp, std::ref(shutdown_server_now), std::ref(should_wait_for_keypress)));	//compiler didn't like lambda here...
	stdin_thread.detach();
	bool should_restart = shutdown_server.get();
	amcp.reset();

	while (weak_amcp.lock());

	return should_restart;
}

void on_abort(int)
{
	CASPAR_THROW_EXCEPTION(invalid_operation() << msg_info("abort called"));
}

void signal_handler(int signum)
{
    ::signal(signum, SIG_DFL);
    boost::stacktrace::safe_dump_to("./backtrace.dump");
    ::raise(SIGABRT);
}

void terminate_handler()
{
    try {
        std::rethrow_exception(std::current_exception());
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
    }
    std::abort();
}

}

int main(int argc, char** argv)
{
    using namespace caspar;

	if (intercept_command_line_args(argc, argv))
		return 0;

    ::signal(SIGSEGV, signal_handler);
    ::signal(SIGABRT, signal_handler);
    std::set_terminate(caspar::terminate_handler);

    static auto backtrace = "./backtrace.dump";
    if (boost::filesystem::exists(backtrace)) {
        std::ifstream ifs(backtrace);
        std::stringstream str;
        str << boost::stacktrace::stacktrace::from_dump(ifs);
        CASPAR_LOG(error) << u16(str.str());
        ifs.close();
        boost::filesystem::remove(backtrace);
    }

	int return_code = 0;
	setup_prerequisites();
	//std::signal(SIGABRT, on_abort);

	setup_global_locale();

	std::wcout << L"Type \"q\" to close application." << std::endl;

	// Set debug mode.
	auto debugging_environment = setup_debugging_environment();

	// Increase process priotity.
	increase_process_priority();

	// Install GPF handler into all tbb threads.
	struct tbb_thread_installer : public tbb::task_scheduler_observer
	{
		tbb_thread_installer(){observe(true);}
		void on_scheduler_entry(bool is_worker) override
		{
		}
	} tbb_thread_installer;

	tbb::task_scheduler_init init;
	std::wstring config_file_name(L"casparcg.config");

	try
	{
		// Configure environment properties from configuration.
		if (argc >= 2)
			config_file_name = caspar::u16(argv[1]);

		env::configure(config_file_name);

		log::set_log_level(env::properties().get(L"configuration.log-level", L"info"));
		auto log_categories_str = env::properties().get(L"configuration.log-categories", L"communication");
		std::set<std::wstring> log_categories;
		boost::split(log_categories, log_categories_str, boost::is_any_of(L", "));
		for (auto& log_category : { L"calltrace", L"communication" })
			log::set_log_category(log_category, log_categories.find(log_category) != log_categories.end());

		if (env::properties().get(L"configuration.debugging.remote", false))
			wait_for_remote_debugging();

		// Start logging to file.
		log::add_file_sink(env::log_folder() + L"caspar",		caspar::log::category != caspar::log::log_category::calltrace);
		log::add_file_sink(env::log_folder() + L"calltrace",	caspar::log::category == caspar::log::log_category::calltrace);
		std::wcout << L"Logging [info] or higher severity to " << env::log_folder() << std::endl << std::endl;

		// Once logging to file, log configuration warnings.
		env::log_configuration_warnings();

		// Setup console window.
		setup_console_window();

		std::atomic<bool> should_wait_for_keypress;
		should_wait_for_keypress = false;
		auto should_restart = run(config_file_name, should_wait_for_keypress);
		return_code = should_restart ? 5 : 0;

		CASPAR_LOG(info) << "Successfully shutdown CasparCG Server.";

		if (should_wait_for_keypress)
			wait_for_keypress();
	}
	catch(boost::property_tree::file_parser_error& e)
	{
		CASPAR_LOG(fatal) << "At " << u8(config_file_name) << ":" << e.line() << ": " << e.message() << ". Please check the configuration file (" << u8(config_file_name) << ") for errors.";
		wait_for_keypress();
	}
	catch (user_error&)
	{
		CASPAR_LOG(fatal) << " Please check the configuration file (" << u8(config_file_name) << ") for errors.";
		wait_for_keypress();
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		CASPAR_LOG(fatal) << L"Unhandled exception in main thread. Please report this error on the CasparCG forums (www.casparcg.com/forum).";
		std::this_thread::sleep_for(std::chrono::milliseconds(1000));
		std::wcout << L"\n\nCasparCG will automatically shutdown. See the log file located at the configured log-file folder for more information.\n\n";
		std::this_thread::sleep_for(std::chrono::milliseconds(4000));
	}

	return return_code;
}
