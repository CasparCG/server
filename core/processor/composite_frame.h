#pragma once

#include "fwd.h"

#include "../format/video_format.h"

#include <memory>
#include <algorithm>

namespace caspar { namespace core {
	
class composite_frame
{
public:
	composite_frame(const std::vector<producer_frame>& frames);
	composite_frame(const producer_frame& frame1, const producer_frame& frame2);

	static std::shared_ptr<composite_frame> interlace(const producer_frame& frame1, const producer_frame& frame2, video_mode::type mode);
	
	const std::vector<short>& audio_data() const;
	std::vector<short>& audio_data();
	
	void begin_write();
	void end_write();
	void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<composite_frame> composite_frame_ptr;
	
}}