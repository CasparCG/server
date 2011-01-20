#pragma once

#include "../fwd.h"

#include "draw_frame.h"

#include "../../core/video_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {

class image_transform;
class audio_transform;
		
class draw_frame
{
public:
	draw_frame();	
	draw_frame(safe_ptr<write_frame>&& image, std::vector<short>&& audio);
	draw_frame(const safe_ptr<draw_frame>& frame);
	draw_frame(safe_ptr<draw_frame>&& frame);
	draw_frame(const std::vector<safe_ptr<draw_frame>>& frames);
	draw_frame(std::vector<safe_ptr<draw_frame>>&& frame);
	draw_frame(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2);
	draw_frame(safe_ptr<draw_frame>&& frame1, safe_ptr<draw_frame>&& frame2);

	void swap(draw_frame& other);
	
	draw_frame(const draw_frame& other);
	draw_frame(draw_frame&& other);

	draw_frame& operator=(const draw_frame& other);
	draw_frame& operator=(draw_frame&& other);
	
	const image_transform& get_image_transform() const;
	image_transform& get_image_transform();

	const audio_transform& get_audio_transform() const;
	audio_transform& get_audio_transform();
		
	static safe_ptr<draw_frame> interlace(const safe_ptr<draw_frame>& frame1, const safe_ptr<draw_frame>& frame2, video_mode::type mode);
		
	static safe_ptr<draw_frame> eof()
	{
		struct eof_frame : public draw_frame{};
		static safe_ptr<draw_frame> frame = make_safe<eof_frame>();
		return frame;
	}

	static safe_ptr<draw_frame> empty()
	{
		struct empty_frame : public draw_frame{};
		static safe_ptr<draw_frame> frame = make_safe<empty_frame>();
		return frame;
	}

	virtual void process_image(image_mixer& mixer);
	virtual void process_audio(audio_mixer& mixer);

	void set_layer_index(int index);
	int get_layer_index() const;
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}