#pragma once

#include <core/channel.h>

#include <common/exception/exceptions.h>

#include <boost/noncopyable.hpp>

#include <vector>

namespace caspar {
	
struct invalid_bootstrapper : virtual boost::exception, virtual std::exception {};

class bootstrapper : boost::noncopyable
{
public:
	bootstrapper();

	const std::vector<safe_ptr<core::channel>> get_channels() const;

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}