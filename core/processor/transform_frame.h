#pragma once

#include "fwd.h"

#include "draw_frame.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {
		
class transform_frame : public draw_frame
{
public:
	transform_frame();
	transform_frame(const safe_ptr<draw_frame>& frame);
	transform_frame(safe_ptr<draw_frame>&& frame);

	void swap(transform_frame& other);
	
	transform_frame(const transform_frame& other);
	transform_frame(transform_frame&& other);

	transform_frame& operator=(const transform_frame& other);
	transform_frame& operator=(transform_frame&& other);
	
	void audio_volume(double volume);
	void translate(double x, double y);
	void texcoord(double left, double top, double right, double bottom);
	void video_mode(video_mode::type mode);
	void alpha(double value);
	
private:

	virtual void process_image(image_processor& processor);
	virtual void process_audio(audio_processor& processor);

	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}