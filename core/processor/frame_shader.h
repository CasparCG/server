#pragma once

#include "../format/video_format.h"
#include "../format/pixel_format.h"

#include <boost/tuple/tuple.hpp>

#include <memory>
#include <array>

namespace caspar { namespace core {
	
struct shader_transform
{
	shader_transform() : alpha(1.0), pos(boost::make_tuple(0.0, 0.0)), uv(boost::make_tuple(0.0, 1.0, 1.0, 0.0)), mode(video_mode::invalid){}
	double alpha;
	boost::tuple<double, double> pos;
	boost::tuple<double, double, double, double> uv;
	video_mode::type mode; 
};

class frame_shader
{
public:
	frame_shader(const video_format_desc& format_desc);

	void begin(const shader_transform& transform);
	void render(const pixel_format_desc& image);
	void end();
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame_shader> frame_shader_ptr;

}}