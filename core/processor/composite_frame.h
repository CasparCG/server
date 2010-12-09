#pragma once

#include "gpu_frame.h"

#include "../format/video_format.h"

#include <memory>
#include <algorithm>

namespace caspar { namespace core {
	
class composite_frame : public gpu_frame
{
public:
	composite_frame(const std::vector<gpu_frame_ptr>& frames);
	composite_frame(const gpu_frame_ptr& frame1, const gpu_frame_ptr& frame2);

	static std::shared_ptr<composite_frame> interlace(const gpu_frame_ptr& frame1, const gpu_frame_ptr& frame2, video_mode::type mode);
	
	virtual const std::vector<short>& audio_data() const;

protected:	
	virtual std::vector<short>& audio_data();

	virtual void begin_write();
	virtual void end_write();
	virtual void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<composite_frame> composite_frame_ptr;
	
}}