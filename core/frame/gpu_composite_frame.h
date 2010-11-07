#pragma once

#include <memory>

#include <Glee.h>

#include "gpu_frame.h"

namespace caspar { namespace core {
	
class gpu_composite_frame : public gpu_frame
{
public:
	gpu_composite_frame();
						
	void add(const gpu_frame_ptr& frame);

	static gpu_frame_ptr interlace(const gpu_frame_ptr& frame1,
									const gpu_frame_ptr& frame2, video_mode mode);
	
private:
	virtual unsigned char* data(size_t index);
	virtual void begin_write();
	virtual void end_write();
	virtual void begin_read();
	virtual void end_read();
	virtual void draw(const gpu_frame_shader_ptr& shader);

	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_composite_frame> gpu_composite_frame_ptr;
	
}}