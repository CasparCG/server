#pragma once

#include <memory>

#include <Glee.h>

#include "gpu_frame.h"

namespace caspar {
	
class composite_gpu_frame : public gpu_frame
{
public:
	composite_gpu_frame(size_t width, size_t height);

	void write_lock();
	bool write_unlock();
	void read_lock(GLenum mode);
	bool read_unlock();
	void draw();
	void reset();
		
	virtual unsigned char* data();
					
	virtual const std::vector<short>& audio_data() const;	
	virtual std::vector<short>& audio_data();

	void add(const gpu_frame_ptr& frame);
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<composite_gpu_frame> composite_gpu_frame_ptr;
	
}