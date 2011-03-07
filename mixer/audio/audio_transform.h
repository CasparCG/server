#pragma once

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

audio_transform lerp(const audio_transform& lhs, const audio_transform& rhs, float alpha);

}}