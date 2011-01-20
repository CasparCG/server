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
	
class image_transform 
{
public:
	image_transform();

	void set_opacity(double value);
	double get_opacity() const;

	void set_gain(double value);
	double get_gain() const;

	void set_position(double x, double y);
	std::array<double, 2> get_position() const;

	void set_size(double x, double ys);
	std::array<double, 2> get_size() const;	

	void set_uv(double left, double top, double right, double bottom);
	std::array<double, 4> get_uv() const;

	void set_mode(video_mode::type mode);
	video_mode::type get_mode() const;

	image_transform& operator*=(const image_transform &other);
	const image_transform operator*(const image_transform &other) const;
private:
	double opacity_;
	double gain_;
	std::array<double, 2> pos_;
	std::array<double, 2> size_;
	std::array<double, 4> uv_;
	video_mode::type mode_;
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