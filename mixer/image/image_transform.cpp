#include "../stdafx.h"

#include "image_transform.h"

#include <common/utility/assert.h>

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

void image_transform::set_fill_translation(double x, double y)
{
	fill_translation_[0] = x;
	fill_translation_[1] = y;
}

void image_transform::set_fill_scale(double x, double y)
{
	fill_scale_[0] = x;
	fill_scale_[1] = y;	
}

std::array<double, 2> image_transform::get_fill_translation() const
{
	return fill_translation_;
}

std::array<double, 2> image_transform::get_fill_scale() const
{
	return fill_scale_;
}

void image_transform::set_key_translation(double x, double y)
{
	key_translation_[0] = x;
	key_translation_[1] = y;
}

void image_transform::set_key_scale(double x, double y)
{
	key_scale_[0] = x;
	key_scale_[1] = y;	
}

std::array<double, 2> image_transform::get_key_translation() const
{
	return key_translation_;
}

std::array<double, 2> image_transform::get_key_scale() const
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
	if(other.mode_ != video_mode::invalid)
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

template<typename T>
T mix(const T& lhs, const T& rhs, float alpha)
{
	return (1.0f - alpha) * lhs + alpha * rhs;
}

image_transform lerp(const image_transform& lhs, const image_transform& rhs, float alpha)
{
	CASPAR_ASSERT(lhs.get_mode() == rhs.get_mode() || lhs.get_mode() == video_mode::invalid || rhs.get_mode() == video_mode::invalid);

	image_transform result;	
	result.set_mode(rhs.get_mode() != video_mode::invalid ? rhs.get_mode() : lhs.get_mode());
	result.set_gain(mix(lhs.get_gain(), rhs.get_gain(), alpha));
	result.set_opacity(mix(lhs.get_opacity(), rhs.get_opacity(), alpha));
	result.set_fill_translation(mix(lhs.get_fill_translation()[0], rhs.get_fill_translation()[0], alpha), mix(lhs.get_fill_translation()[1], rhs.get_fill_translation()[1], alpha));
	result.set_fill_scale(mix(lhs.get_fill_scale()[0], rhs.get_fill_scale()[0], alpha), mix(lhs.get_fill_scale()[1], rhs.get_fill_scale()[1], alpha));
	result.set_key_translation(mix(lhs.get_key_translation()[0], rhs.get_key_translation()[0], alpha), mix(lhs.get_key_translation()[1], rhs.get_key_translation()[1], alpha));
	result.set_key_scale(mix(lhs.get_key_scale()[0], rhs.get_key_scale()[0], alpha), mix(lhs.get_key_scale()[1], rhs.get_key_scale()[1], alpha));
	
	return result;
}

}}