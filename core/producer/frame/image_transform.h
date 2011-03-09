#pragma once

#include <core/video_format.h>

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
	
	void set_fill_translation(double x, double y);
	std::array<double, 2> get_fill_translation() const;

	void set_fill_scale(double x, double y);
	std::array<double, 2> get_fill_scale() const;
	
	void set_key_translation(double x, double y);
	std::array<double, 2> get_key_translation() const;

	void set_key_scale(double x, double y);
	std::array<double, 2> get_key_scale() const;

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

image_transform lerp(const image_transform& lhs, const image_transform& rhs, float alpha);

inline bool operator==(const image_transform& lhs, const image_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(image_transform)) == 0;
}

inline bool operator!=(const image_transform& lhs, const image_transform& rhs)
{
	return !(lhs == rhs);
}

}}