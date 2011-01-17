#pragma once

#include "../gpu/host_buffer.h"

#include "../../video_format.h"

#include <boost/tuple/tuple.hpp>
#include <boost/thread/future.hpp>
#include <boost/noncopyable.hpp>

#include <memory>
#include <array>

namespace caspar { namespace core {

struct pixel_format_desc;
	
struct image_transform 
{
	image_transform() : alpha(1.0), pos(boost::make_tuple(0.0, 0.0)), gain(1.0), uv(boost::make_tuple(0.0, 0.0, 0.0, 0.0)), mode(video_mode::invalid){}
	double alpha;
	double gain;
	boost::tuple<double, double> pos;
	boost::tuple<double, double, double, double> uv;
	video_mode::type mode; 

	image_transform& operator*=(const image_transform &other);
	const image_transform operator*(const image_transform &other) const;
};

class image_mixer : boost::noncopyable
{
public:
	image_mixer(const video_format_desc& format_desc);

	void begin(const image_transform& transform);
	void process(const pixel_format_desc& desc, std::vector<safe_ptr<host_buffer>>& buffers);
	void end();

	boost::unique_future<safe_ptr<const host_buffer>> begin_pass();
	void end_pass();

	std::vector<safe_ptr<host_buffer>> create_buffers(const pixel_format_desc& format);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}