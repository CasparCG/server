#pragma once

#include "fwd.h"

#include "drawable_frame.h"

#include "../format/video_format.h"

#include <memory>
#include <algorithm>

namespace caspar { namespace core {
	
class composite_frame : public drawable_frame
{
public:
	composite_frame(const std::vector<producer_frame>& frames);
	composite_frame(const producer_frame& frame1, const producer_frame& frame2);
	composite_frame(composite_frame&& other);
	composite_frame& operator=(composite_frame&& other);

	static composite_frame interlace(producer_frame&& frame1, producer_frame&& frame2, video_mode::type mode);
	
	virtual const std::vector<short>& audio_data() const;
	virtual std::vector<short>& audio_data();
	
	virtual void begin_write();
	virtual void end_write();
	virtual void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
	
}}