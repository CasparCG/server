#pragma once

#include <tbb/spin_mutex.h>

namespace caspar { namespace core {

class audio_transform
{
public:
	audio_transform();

	void set_gain(double value);
	double get_gain() const;

	audio_transform& operator*=(const audio_transform &other);
	const audio_transform operator*(const audio_transform &other) const;
private:
	double gain_;
	mutable tbb::spin_mutex mutex_;
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