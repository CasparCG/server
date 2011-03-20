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

audio_transform tween(double time, const audio_transform& source, const audio_transform& dest, double duration, const tweener_t& tweener)
{
	auto do_tween = [](double time, double source, double dest, double duration, const tweener_t& tweener)
	{
		return tweener(time, source, dest-source, duration);
	};

	audio_transform result;
	result.set_gain(do_tween(time, source.get_gain(), dest.get_gain(), duration, tweener));
	result.set_has_audio(source.get_has_audio() || dest.get_has_audio());
	return result;
}

}}