#include "../../../StdAfx.h"

#include "console_server.h"

#include <boost/signals2.hpp>

namespace caspar { namespace controller { namespace io { namespace console {

struct console_server::implementation
{
	boost::signals2::connection subscribe(const std::function<void(const std::wstring& message, int tag)>& func)
	{
		return signal_.connect(func);
	}

	void start_write(const std::wstring& message)
	{
		std::wcout << message << std::endl;
	}
	
	void run()
	{
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
			signal_(wcmd, 0);
		}
	}

	boost::signals2::signal<void(const std::wstring& message, int tag)> signal_;
};

console_server::console_server() : impl_(new implementation){}
boost::signals2::connection console_server::subscribe(const std::function<void(const std::wstring& message, int tag)>& func) { return impl_->subscribe(func); }
void console_server::start_write(const std::wstring& message, int tag) { impl_->start_write(message); }
void console_server::run() { impl_->run(); }

}}}}
