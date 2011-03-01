#pragma once

#include "../gpu/host_buffer.h"

#include <core/video_format.h>

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
	
	void set_image_translation(double x, double y);
	std::array<double, 2> get_image_translation() const;

	void set_image_scale(double x, double y);
	std::array<double, 2> get_image_scale() const;
	
	void set_mask_translation(double x, double y);
	std::array<double, 2> get_mask_translation() const;

	void set_mask_scale(double x, double y);
	std::array<double, 2> get_mask_scale() const;

	void set_mode(video_mode::type mode);
	video_mode::type get_mode() const;

	image_transform& operator*=(const image_transform &other);
	const image_transform operator*(const image_transform &other) const;
private:
	double opacity_;
	double gain_;
	std::array<double, 2> fill_translation_; 
	std::array<double, 2> fill_scale_; 
	std::array<double, 2> key_translation_; 
	std::array<double, 2> key_scale_; 
	video_mode::type mode_;
};

}}