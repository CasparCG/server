#pragma once

#include "fwd.h"

#include "drawable_frame.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class transform_frame : public drawable_frame
{
public:
	transform_frame(const producer_frame& frame);
	transform_frame(transform_frame&& other);
	transform_frame& operator=(transform_frame&& other);
	
	virtual std::vector<short>& audio_data();
	virtual const std::vector<short>& audio_data() const;

	void audio_volume(unsigned char volume);
	void translate(double x, double y);
	void texcoord(double left, double top, double right, double bottom);
	void video_mode(video_mode::type mode);
	void alpha(double value);

	virtual void begin_write();
	virtual void end_write();
	virtual void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}