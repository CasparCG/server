#pragma once

#include <boost/noncopyable.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
struct drawable_frame : boost::noncopyable
{	
	virtual const std::vector<short>& audio_data() const = 0;
	virtual std::vector<short>& audio_data() = 0;
			
	virtual void begin_write() = 0;
	virtual void end_write() = 0;
	virtual void draw(frame_shader& shader) = 0;
};


}}