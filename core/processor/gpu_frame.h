#pragma once

#include "fwd.h"

#include "../../common/exception/exceptions.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {
		
class gpu_frame : boost::noncopyable
{	
public:
	virtual const std::vector<short>& audio_data() const = 0;
	virtual std::vector<short>& audio_data() = 0;
	
	virtual void begin_write() = 0;
	virtual void end_write() = 0;
	virtual void draw(frame_shader& shader) = 0;
};
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;


}}