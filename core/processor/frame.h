#pragma once

#include "frame_shader.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <memory>
#include <array>

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>

#include <vector>

namespace caspar { namespace core {
	
class frame : boost::noncopyable
{
public:
	
	struct render_transform
	{
		render_transform() : alpha(1.0), pos(boost::make_tuple(0.0, 0.0)), uv(boost::make_tuple(0.0, 1.0, 1.0, 0.0)), mode(video_mode::progressive){}
		double alpha;
		boost::tuple<double, double> pos;
		boost::tuple<double, double, double, double> uv;
		video_mode::type mode; 
	};

	virtual ~frame(){}
			
	virtual unsigned char* data(size_t index = 0);
	virtual size_t size(size_t index = 0) const;
				
	virtual std::vector<short>& get_audio_data();
	const std::vector<short>& get_audio_data() const;
	
	virtual render_transform& get_render_transform();
	const render_transform& get_render_transform() const;
	
	static std::shared_ptr<frame>& empty()
	{
		static auto empty_frame = std::shared_ptr<frame>(new frame(pixel_format_desc()));
		return empty_frame;
	}

protected:
	frame(const pixel_format_desc& desc);

	friend class frame_processor_device;
	friend class frame_renderer;

	virtual void reset();
		
	virtual void begin_write();
	virtual void end_write();
	virtual void begin_read();
	virtual void end_read();

	virtual void draw(frame_shader& shader);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame> frame_ptr;
	
}}