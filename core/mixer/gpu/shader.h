#pragma once

#include <string>

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {
		
class shader : boost::noncopyable
{
public:
	shader(const std::string& vertex_source_str, const std::string& fragment_source_str);
	void set(const std::string& name, int value);
	void set(const std::string& name, float value);
	void set(const std::string& name, double value);
private:
	friend class ogl_device;
	struct implementation;
	safe_ptr<implementation> impl_;

	int id() const;
};

}}