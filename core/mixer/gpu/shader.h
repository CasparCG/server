#pragma once

#include <string>

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
		
class shader : boost::noncopyable
{
public:
	shader(const std::string& vertex_source_str, const std::string& fragment_source_str);
	void use();
	void set(const std::string& name, int value);
	void set(const std::string& name, float value);
private:
	struct implementation;
	safe_ptr<implementation> impl_;
};

}}