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

}}