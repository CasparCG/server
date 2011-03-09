#include "../../stdafx.h"

#include "audio_transform.h"

namespace caspar { namespace core {
	
audio_transform::audio_transform()
	: gain_(1.0)
	, audio_(true){}

void audio_transform::set_gain(double value)
{
	gain_ = value;
}

double audio_transform::get_gain() const
{
	return gain_;
}

void audio_transform::set_has_audio(bool value)
{
	audio_ = value;
}

bool audio_transform::get_has_audio() const
{
	return audio_;
}

audio_transform& audio_transform::operator*=(const audio_transform &other) 
{
	gain_ *= other.gain_;
	audio_ &= other.audio_;
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
	result.set_has_audio(lhs.get_has_audio() || rhs.get_has_audio());
	return result;
}

}}