#pragma once

#include <common/utility/tweener.h>

namespace caspar { namespace core {

class audio_transform
{
public:
	audio_transform();

	void set_gain(double value);
	double get_gain() const;
	
	void set_has_audio(bool value);
	bool get_has_audio() const;

	audio_transform& operator*=(const audio_transform &other);
	const audio_transform operator*(const audio_transform &other) const;
private:
	double gain_;
	bool audio_;
};

audio_transform tween(double time, const audio_transform& source, const audio_transform& dest, double duration, const tweener_t& tweener);

}}