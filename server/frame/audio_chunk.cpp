#include "../StdAfx.h"

#include "audio_chunk.h"

#include <tbb/scalable_allocator.h>

namespace caspar{

audio_chunk::audio_chunk(size_t dataSize, const sound_channel_info_ptr& snd_channel_info) 
	: size_(dataSize), snd_channel_info_(snd_channel_info), data_(static_cast<unsigned char*>(scalable_aligned_malloc(dataSize, 16))), volume_(100.0f)
{}

audio_chunk::~audio_chunk() 
{
	scalable_aligned_free(data_);
}

const unsigned char* audio_chunk::data() const
{
	return data_;
}

unsigned char* audio_chunk::data()
{
	return data_;
}

size_t audio_chunk::size() const
{
	return size_;
}

float audio_chunk::volume() const
{
	return volume_;
}

void audio_chunk::set_volume(float volume)
{
	assert(volume_ > 0.0f);
	assert(volume_ < (100.0f + std::numeric_limits<float>::epsilon()));
	volume_ = volume;
}
	
const sound_channel_info_ptr& audio_chunk::sound_channel_info() const
{
	return snd_channel_info_;
}

}