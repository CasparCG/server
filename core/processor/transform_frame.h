#pragma once

#include "fwd.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class transform_frame
{
public:
	explicit transform_frame(const producer_frame& frame);
	
	std::vector<short>& audio_data();
	const std::vector<short>& audio_data() const;

	void audio_volume(unsigned char volume);
	void translate(double x, double y);
	void texcoord(double left, double top, double right, double bottom);
	void video_mode(video_mode::type mode);
	void alpha(double value);

	void begin_write();
	void end_write();
	void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<transform_frame> transform_frame_ptr;

}}