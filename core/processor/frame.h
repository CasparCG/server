#pragma once

#include "frame_shader.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <memory>
#include <array>

#include <boost/noncopyable.hpp>
#include <boost/tuple/tuple.hpp>
#include <boost/range/iterator_range.hpp>

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
			
	virtual boost::iterator_range<unsigned char*> data(size_t index = 0) = 0;
				
	virtual std::vector<short>& get_audio_data() = 0;
	const std::vector<short>& get_audio_data() const { const_cast<frame*>(this)->get_audio_data();}
	
	virtual render_transform& get_render_transform() = 0;
	const render_transform& get_render_transform() const  { const_cast<frame*>(this)->get_render_transform();}
	
	static std::shared_ptr<frame>& empty();
};
typedef std::shared_ptr<frame> frame_ptr;
	
class internal_frame : public frame
{
public:
	internal_frame(const pixel_format_desc& desc);
	
	virtual boost::iterator_range<unsigned char*> data(size_t index = 0);

	virtual std::vector<short>& get_audio_data();
	virtual core::frame::render_transform& get_render_transform();

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
typedef std::shared_ptr<internal_frame> internal_frame_ptr;


}}