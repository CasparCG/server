#pragma once

namespace caspar { namespace core {

struct audio_transform
{
	audio_transform() : volume(1.0){}
	double volume;

	audio_transform& operator*=(const audio_transform &other);
	const audio_transform operator*(const audio_transform &other) const;
};

class audio_processor
{
public:
	audio_processor();

	void begin(const audio_transform& transform);
	void process(const std::vector<short>& audio_data);
	void end();

	std::vector<short> read();

	void begin_pass(){}
	void end_pass(){}

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}