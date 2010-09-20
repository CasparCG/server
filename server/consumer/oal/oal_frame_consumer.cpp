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
 
#include "../../StdAfx.h"

#include "oal_frame_consumer.h"

#include "../../frame/audio_chunk.h"
#include "../../frame/frame.h"
#include "../../frame/format.h"

#include <SFML/Audio.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/foreach.hpp>

#include <windows.h>

namespace caspar{ namespace audio{	

class sound_channel : public sf::SoundStream
{
public:
	sound_channel() : internal_chunks_(5)
	{
		external_chunks_.set_capacity(3);
	}

	~sound_channel()
	{
		external_chunks_.clear();
		external_chunks_.push(nullptr);
		Stop();
	}

	void Initialize(const sound_channel_info_ptr& info)
	{
		sf::SoundStream::Initialize(info->channels_count, info->sample_rate);
		assert(info->bits_per_sample/(8*sizeof(char)) == sizeof(sf::Int16));
	}

	void Push(const audio_chunk_ptr& paudio_chunk)
	{
		if(!external_chunks_.try_push(paudio_chunk))
		{
			//CASPAR_LOG(debug) << "Sound Buffer Overrun";
			external_chunks_.push(paudio_chunk);
		}

		if(GetStatus() != Playing && external_chunks_.size() >= 3)
			Play();
	}

private:

	bool OnStart() 
	{
		external_chunks_.clear();
		return true;
	}

	bool OnGetData(sf::SoundStream::Chunk& data)
    {
		audio_chunk_ptr pChunk;
		if(!external_chunks_.try_pop(pChunk))
		{
#ifdef CASPAR_TRACE_UNDERFLOW
			CASPAR_LOG(trace) << "Sound Buffer Underrun";
#endif
			external_chunks_.pop(pChunk);
		}

		if(pChunk == nullptr)
		{
			external_chunks_.clear();
			return false;
		}

		internal_chunks_.push_back(pChunk);
		SetVolume(pChunk->volume());
		data.Samples = reinterpret_cast<sf::Int16*>(pChunk->data());
		data.NbSamples = pChunk->size()/sizeof(sf::Int16);
        return true;
    }

	boost::circular_buffer<audio_chunk_ptr> internal_chunks_;
	tbb::concurrent_bounded_queue<audio_chunk_ptr> external_chunks_;
};
typedef std::shared_ptr<sound_channel> sound_channelPtr;

struct oal_frame_consumer::implementation : boost::noncopyable
{	
	implementation(const frame_format_desc& format_desc) : format_desc_(format_desc)
	{
		for(int n = 0; n < 3; ++n)
			channel_pool_.push(std::make_shared<sound_channel>());
	}
	
	void push(const frame_ptr& frame)
	{
		if(frame == nullptr)
			return;

		decltype(channels_) active_channels;
		BOOST_FOREACH(const audio_chunk_ptr& pChunk, frame->audio_data())
		{
			auto info = pChunk->sound_channel_info();
			auto it = channels_.find(info);
			sound_channelPtr channel;
			if(it == channels_.end())
			{
				if(channel_pool_.size() <= 1)
					channel_pool_.push(std::make_shared<sound_channel>());
				
				sound_channelPtr pNewChannel;
				channel_pool_.pop(pNewChannel);

				channel = sound_channelPtr(pNewChannel.get(), [=](sound_channel*)
				{
					channel_pool_.push(pNewChannel);
					pNewChannel->Push(nullptr);
				});
				channel->Initialize(info);
			}
			else
				channel = it->second;

			active_channels.insert(std::make_pair(info, channel));
			channel->Push(pChunk); // Could Block
		}

		channels_ = std::move(active_channels);
	}
		
	tbb::concurrent_bounded_queue<sound_channelPtr> channel_pool_;
	std::map<sound_channel_info_ptr, sound_channelPtr> channels_;

	caspar::frame_format_desc format_desc_;
};

oal_frame_consumer::oal_frame_consumer(const caspar::frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
const caspar::frame_format_desc& oal_frame_consumer::get_frame_format_desc() const{return impl_->format_desc_;}
void oal_frame_consumer::prepare(const frame_ptr& frame){impl_->push(frame);}
}}
