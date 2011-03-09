#pragma once

#include "../fwd.h"

#include "frame_visitor.h"

#include <common/memory/safe_ptr.h>
#include <core/video_format.h>

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>

#include <memory>
#include <array>
#include <vector>

namespace caspar { namespace core {

class image_transform;
class audio_transform;
		
class basic_frame
{
public:
	basic_frame();	
	basic_frame(safe_ptr<write_frame>&& image, std::vector<short>&& audio);
	basic_frame(const safe_ptr<basic_frame>& frame);
	basic_frame(safe_ptr<basic_frame>&& frame);
	basic_frame(const std::vector<safe_ptr<basic_frame>>& frames);
	basic_frame(std::vector<safe_ptr<basic_frame>>&& frame);
	basic_frame(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2);
	basic_frame(safe_ptr<basic_frame>&& frame1, safe_ptr<basic_frame>&& frame2);

	void swap(basic_frame& other);
	
	basic_frame(const basic_frame& other);
	basic_frame(basic_frame&& other);

	basic_frame& operator=(const basic_frame& other);
	basic_frame& operator=(basic_frame&& other);
	
	const image_transform& get_image_transform() const;
	image_transform& get_image_transform();

	const audio_transform& get_audio_transform() const;
	audio_transform& get_audio_transform();
		
	static safe_ptr<basic_frame> interlace(const safe_ptr<basic_frame>& frame1, const safe_ptr<basic_frame>& frame2, video_mode::type mode);
		
	static const safe_ptr<basic_frame>& eof()
	{
		struct eof_frame : public basic_frame{};
		static safe_ptr<basic_frame> frame = make_safe<eof_frame>();
		return frame;
	}

	static const safe_ptr<basic_frame>& empty()
	{
		struct empty_frame : public basic_frame{};
		static safe_ptr<basic_frame> frame = make_safe<empty_frame>();
		return frame;
	}
	
	virtual void accept(frame_visitor& visitor);

	void set_layer_index(int index);
	int get_layer_index() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}