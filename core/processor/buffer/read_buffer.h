#pragma once

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {
	
class read_buffer 
{
	friend class image_processor;
public:
	read_buffer(size_t width, size_t height);
	
	const void* data() const;
	size_t size() const;	
	size_t width() const;
	size_t height() const;
		
	void unmap();
	void map();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}