/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar{
	
struct sound_channel_info : boost::noncopyable
{
public:
	sound_channel_info(size_t channels_count, size_t bits_per_sample, size_t sample_rate)
		: channels_count(channels_count), bits_per_sample(bits_per_sample), sample_rate(sample_rate){}
	const size_t channels_count;
	const size_t bits_per_sample;
	const size_t sample_rate;
};
typedef std::shared_ptr<sound_channel_info> sound_channel_info_ptr;
typedef std::unique_ptr<sound_channel_info> sound_channel_info_uptr;

class audio_chunk : boost::noncopyable
{
public:
	audio_chunk(size_t dataSize, const sound_channel_info_ptr& snd_channel_info);
	~audio_chunk();

	const unsigned char* data() const;
	unsigned char* data();
	size_t size() const;
	float volume() const;
	void set_volume(float volume);
	const sound_channel_info_ptr& sound_channel_info() const;

private:
	float volume_;
	const sound_channel_info_ptr snd_channel_info_;
	unsigned char* const data_;
	const int size_;
};
typedef std::shared_ptr<audio_chunk> audio_chunk_ptr;
typedef std::unique_ptr<audio_chunk> audio_chunk_uptr;

}