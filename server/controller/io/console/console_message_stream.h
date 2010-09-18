#pragma once

#include <string>

#include <boost/signals2/connection.hpp>

namespace caspar { namespace controller { namespace io { namespace console {

class console_message_stream
{
public:
	console_message_stream();

	boost::signals2::connection subscribe(const std::function<void(const std::wstring& message)>& func);
	void send(const std::wstring& message);

	void run();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}}
