#pragma once

#include <memory>

#include <Glee.h>

#include "gpu_frame.h"

namespace caspar { namespace core {
	
class composite_gpu_frame : public gpu_frame
{
public:
	composite_gpu_frame(size_t width, size_t height);

	void write_lock();
	bool write_unlock();
	void read_lock(GLenum mode);
	bool read_unlock();
	void draw();

	virtual unsigned char* data();
					
	void add(const gpu_frame_ptr& frame);

	static gpu_frame_ptr interlace(const gpu_frame_ptr& frame1 ,const gpu_frame_ptr& frame2, video_mode mode);
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<composite_gpu_frame> composite_gpu_frame_ptr;
	
}}