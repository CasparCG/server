#pragma once

#include <memory>

#include <Glee.h>

#include "gpu_frame.h"

namespace caspar { namespace core {
	
class gpu_composite_frame : public gpu_frame
{
public:
	gpu_composite_frame(size_t width, size_t height);

	virtual void write_lock();
	virtual bool write_unlock();
	virtual void read_lock(GLenum mode);
	virtual bool read_unlock();
	virtual void draw();
					
	void add(const gpu_frame_ptr& frame);

	static gpu_frame_ptr interlace(const gpu_frame_ptr& frame1 ,const gpu_frame_ptr& frame2, video_mode mode);
	
private:
	virtual unsigned char* data();

	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_composite_frame> gpu_composite_frame_ptr;
	
}}