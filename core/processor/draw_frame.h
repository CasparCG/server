#pragma once

#include "fwd.h"

#include "draw_frame.h"

#include "../format/video_format.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {

struct image_transform;
struct audio_transform;
		
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
	
	void audio_volume(double volume);
	void translate(double x, double y);
	void texcoord(double left, double top, double right, double bottom);
	void video_mode(video_mode::type mode);
	void alpha(double value);

	const image_transform& get_image_transform() const;
	const audio_transform& get_audio_transform() const;
		
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

	virtual void process_image(image_processor& processor);
	virtual void process_audio(audio_processor& processor);
	
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}