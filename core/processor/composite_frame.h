#pragma once

#include "fwd.h"

#include "draw_frame.h"

#include "../format/video_format.h"

#include <memory>
#include <algorithm>

namespace caspar { namespace core {
	
class composite_frame : public detail::draw_frame_impl
{
public:
	explicit composite_frame(std::vector<draw_frame>&& frames);
	composite_frame(draw_frame&& frame1, draw_frame&& frame2);
	
	void swap(composite_frame& other);

	composite_frame(const composite_frame& other);
	composite_frame& operator=(const composite_frame& other);
	composite_frame(composite_frame&& other);
	composite_frame& operator=(composite_frame&& other);

	static composite_frame interlace(draw_frame&& frame1, draw_frame&& frame2, video_mode::type mode);
	
	virtual const std::vector<short>& audio_data() const;

private:	
	virtual void begin_write();
	virtual void end_write();
	virtual void draw(frame_shader& shader);

	struct implementation;
	std::shared_ptr<implementation> impl_;
};
	
}}