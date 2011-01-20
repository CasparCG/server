#pragma once

#include <tbb/spin_mutex.h>

namespace caspar { namespace core {

class audio_transform;

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