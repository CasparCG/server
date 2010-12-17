#pragma once

#include "read_frame.h"

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/tuple/tuple.hpp>

#include <memory>
#include <array>

namespace caspar { namespace core {
	
struct image_transform
{
	image_transform() : alpha(1.0), pos(boost::make_tuple(0.0, 0.0)), uv(boost::make_tuple(0.0, 1.0, 1.0, 0.0)), mode(video_mode::invalid){}
	double alpha;
	boost::tuple<double, double> pos;
	boost::tuple<double, double, double, double> uv;
	video_mode::type mode; 
};

class image_processor
{
public:
	image_processor(const video_format_desc& format_desc);

	void begin(const image_transform& transform);
	void process(const pixel_format_desc& desc, std::vector<gl::pbo>& pbos);
	void end();

	safe_ptr<read_frame> begin_pass();
	void end_pass();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
	video_format_desc format_desc_;
};
typedef std::shared_ptr<image_processor> image_shader_ptr;

}}