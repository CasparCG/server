#pragma once

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {
		
class host_buffer : boost::noncopyable
{
public:
	enum usage_t
	{
		write_only,
		read_only
	};
	host_buffer(size_t size, usage_t usage);
	
	const void* data() const;
	void* data();
	size_t size() const;	
	
	void bind();
	void unbind();
	void unmap();
	void map();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}