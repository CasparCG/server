#include "../../StdAfx.h"

#include "image_transform.h"

namespace caspar { namespace core {
		
image_transform::image_transform() 
	: opacity_(1.0)
	, gain_(1.0)
	, mode_(video_mode::invalid)
{
	std::fill(fill_translation_.begin(), fill_translation_.end(), 0.0);
	std::fill(fill_scale_.begin(), fill_scale_.end(), 1.0);
	std::fill(key_translation_.begin(), key_translation_.end(), 0.0);
	std::fill(key_scale_.begin(), key_scale_.end(), 1.0);
}

void image_transform::set_opacity(double value)
{
	opacity_ = value;
}

double image_transform::get_opacity() const
{
	return opacity_;
}

void image_transform::set_gain(double value)
{
	gain_ = value;
}

double image_transform::get_gain() const
{
	return gain_;
}

void image_transform::set_image_translation(double x, double y)
{
	fill_translation_[0] = x;
	fill_translation_[1] = y;
}

void image_transform::set_image_scale(double x, double y)
{
	fill_scale_[0] = x;
	fill_scale_[1] = y;	
}

std::array<double, 2> image_transform::get_image_translation() const
{
	return fill_translation_;
}

std::array<double, 2> image_transform::get_image_scale() const
{
	return fill_scale_;
}

void image_transform::set_mask_translation(double x, double y)
{
	key_translation_[0] = x;
	key_translation_[1] = y;
}

void image_transform::set_mask_scale(double x, double y)
{
	key_scale_[0] = x;
	key_scale_[1] = y;	
}

std::array<double, 2> image_transform::get_mask_translation() const
{
	return key_translation_;
}

std::array<double, 2> image_transform::get_mask_scale() const
{
	return key_scale_;
}

void image_transform::set_mode(video_mode::type mode)
{
	mode_ = mode;
}

video_mode::type image_transform::get_mode() const
{
	return mode_;
}

image_transform& image_transform::operator*=(const image_transform &other)
{
	opacity_ *= other.opacity_;
	mode_ = other.mode_;
	gain_ *= other.gain_;
	fill_translation_[0] += other.fill_translation_[0]*fill_scale_[0];
	fill_translation_[1] += other.fill_translation_[1]*fill_scale_[1];
	fill_scale_[0] *= other.fill_scale_[0];
	fill_scale_[1] *= other.fill_scale_[1];
	key_translation_[0] += other.key_translation_[0]*key_scale_[0];
	key_translation_[1] += other.key_translation_[1]*key_scale_[1];
	key_scale_[0] *= other.key_scale_[0];
	key_scale_[1] *= other.key_scale_[1];
	return *this;
}

const image_transform image_transform::operator*(const image_transform &other) const
{
	return image_transform(*this) *= other;
}

}}