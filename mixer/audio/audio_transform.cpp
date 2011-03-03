#include "../stdafx.h"

#include "audio_transform.h"

namespace caspar { namespace core {
	
audio_transform::audio_transform()
	: gain_(1.0){}

void audio_transform::set_gain(double value)
{
	gain_ = value;
}

double audio_transform::get_gain() const
{
	return gain_;
}

audio_transform& audio_transform::operator*=(const audio_transform &other) 
{
	gain_ *= other.gain_;
	return *this;
}

const audio_transform audio_transform::operator*(const audio_transform &other) const
{
	return audio_transform(*this) *= other;
}

template<typename T>
T mix(const T& lhs, const T& rhs, float alpha)
{
	return (1.0f - alpha) * lhs + alpha * rhs;
}

audio_transform lerp(const audio_transform& lhs, const audio_transform& rhs, float alpha)
{
	audio_transform result;
	result.set_gain(mix(lhs.get_gain(), rhs.get_gain(), alpha));
	return result;
}

}}