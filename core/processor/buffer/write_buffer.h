#pragma once

#include <memory>

#include <boost/noncopyable.hpp>

#include <Glee.h>

#include <boost/tuple/tuple.hpp>
#include <boost/thread/future.hpp>

namespace caspar { namespace core {
	
class write_buffer : boost::noncopyable
{
	friend class image_processor;
public:

	write_buffer(write_buffer&& other);
	write_buffer(size_t width, size_t height, size_t depth);
	write_buffer& operator=(write_buffer&& other);
	
	void* data();
	size_t size() const;
	size_t width() const;
	size_t height() const;
				
	void map();
	void unmap();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
}}