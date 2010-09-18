#pragma once

#include <string>

#include <boost/signals2/connection.hpp>

namespace caspar { namespace controller { namespace io { namespace console {

class console_server
{
public:
	console_server();

	boost::signals2::connection subscribe(const std::function<void(const std::wstring& message, int tag)>& func);
	void start_write(const std::string& message, int tag);

	void run();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}}}
