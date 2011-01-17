#pragma once

namespace caspar { namespace core {

struct audio_transform
{
	audio_transform() : gain(1.0){}
	double gain;

	audio_transform& operator*=(const audio_transform &other);
	const audio_transform operator*(const audio_transform &other) const;
};

class audio_mixer
{
public:
	audio_mixer();

	void begin(const audio_transform& transform);
	void process(const std::vector<short>& audio_data);
	void end();
	
	std::vector<short> begin_pass();
	void end_pass();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}