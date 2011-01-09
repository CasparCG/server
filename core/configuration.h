#pragma once

#include "channel.h"

#include <common/exception/exceptions.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar { namespace core { 
	
struct invalid_configuration : virtual boost::exception, virtual std::exception {};

class configuration : boost::noncopyable
{
public:
	configuration();

	static const std::wstring& media_folder();
	static const std::wstring& log_folder();
	static const std::wstring& template_folder();		
	static const std::wstring& data_folder();

	const std::vector<safe_ptr<channel>> get_channels() const;

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}