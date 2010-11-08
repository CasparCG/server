#pragma once

#include "channel.h"

#include "producer/frame_producer_device.h"

#include <common/exception/exceptions.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar { namespace core { 
	
struct invalid_configuration : virtual boost::exception, virtual std::exception {};

class server : boost::noncopyable
{
public:
	server();

	static const std::wstring& media_folder();
	static const std::wstring& log_folder();
	static const std::wstring& template_folder();		
	static const std::wstring& data_folder();

	const std::vector<channel_ptr> get_channels() const;

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}