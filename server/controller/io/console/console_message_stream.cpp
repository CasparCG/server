#include "../../../StdAfx.h"

#include "console_message_stream.h"

#include <boost/signals2.hpp>

namespace caspar { namespace controller { namespace io { namespace console {

struct console_message_stream::implementation
{
	boost::signals2::connection subscribe(const std::function<void(const std::wstring& message)>& func)
	{
		return signal_.connect(func);
	}

	void send(const std::wstring& message)
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
			signal_(wcmd);
		}
	}

	boost::signals2::signal<void(const std::wstring& message)> signal_;
};

console_message_stream::console_message_stream() : impl_(new implementation){}
boost::signals2::connection console_message_stream::subscribe(const std::function<void(const std::wstring& message)>& func) { return impl_->subscribe(func); }
void console_message_stream::send(const std::wstring& message) { impl_->send(message); }
void console_message_stream::run() { impl_->run(); }

}}}}
