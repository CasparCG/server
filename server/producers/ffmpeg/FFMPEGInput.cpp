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
 
#include "../../stdafx.h"

#include "FFMPEGInput.h"

#include "FFMPEGException.h"
#include "video/FFMPEGVideoDecoder.h"
#include "audio/FFMPEGAudioDecoder.h"

#include "../../utils/Logger.h"
#include "../../utils/LogLevel.h"
#include "../../utils/image/Image.hpp"
#include "../../utils/UnhandledException.h"
#include "../../utils/scope_exit.h"

#include <tbb/concurrent_queue.h>
#include <tbb/pipeline.h>
#include <tbb/spin_mutex.h>

#include <errno.h>
#include <system_error>
#include <algorithm>

#pragma warning(disable : 4482)

namespace caspar
{
	namespace ffmpeg
	{
		
struct InputFilter::Implementation
{
	Implementation() 
		: 
		videoFrameRate_(25.0),
		videoStreamIndex_(-1),
		audioStreamIndex_(-1),
		pVideoCodec_(nullptr),
		pAudioCodec_(nullptr)
	{
		loop_ = false;
		isRunning_ = false;
	}

	void Load(const std::string& filename, std::error_code& errc = std::error_code())
	{	
		isRunning_ = false;

		CASPAR_SCOPE_EXIT([&]
		{
			if(!isRunning_)
			{
				pVideoCodecContext_.reset();
				pAudioCodecContext_.reset();
				pFormatContext_.reset();
				videoFrameRate_ = 25.0;
				videoStreamIndex_ = -1;
				audioStreamIndex_ = -1;
			}
		});

		AVFormatContext* pWeakFormatContext;
		errc = ffmpeg_make_averror_code(av_open_input_file(&pWeakFormatContext, filename.c_str(), nullptr, 0, nullptr));
		pFormatContext_.reset(pWeakFormatContext, av_close_input_file);
		if(errc)
			throw ffmpeg_error(errc);
			
		errc = ffmpeg_make_averror_code(av_find_stream_info(pFormatContext_.get()));
		if(errc)
			throw ffmpeg_error(errc);

		pVideoCodecContext_ = OpenVideoStream();
			
		videoFrameRate_ = static_cast<double>(pVideoCodecContext_->time_base.den) / static_cast<double>(pVideoCodecContext_->time_base.num);

		try
		{
			pAudioCodecContext_ = OpenAudioStream();
		}
		catch(std::system_error& er) // Audio is non critical
		{
			errc = er.code();
			LOG << utils::LogLevel::Verbose 
				<< TEXT("[ffmpeg::InputFilter] Failed to Load AudioStream. Continuing without Audio. Error: " 
				<< errc.message().c_str());
		}

		isRunning_ = true;
	}
		
	std::shared_ptr<AVCodecContext> OpenVideoStream()
	{
		bool succeeded = false;

		CASPAR_SCOPE_EXIT([&]
		{
			if(!succeeded)
			{
				videoStreamIndex_ = -1;
				pVideoCodec_ = nullptr;
			}
		});

		AVStream** streams_end = pFormatContext_->streams+pFormatContext_->nb_streams;
		AVStream** videoStream = std::find_if(pFormatContext_->streams, streams_end, 
			[](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == CODEC_TYPE_VIDEO ;});

		videoStreamIndex_ = videoStream != streams_end ? (*videoStream)->index : -1;
		if(videoStreamIndex_ == -1) 
			throw ffmpeg_error(ffmpeg_make_error_code(ffmpeg_errc::no_streams));
		
		pVideoCodec_ = avcodec_find_decoder((*videoStream)->codec->codec_id);			
		if(pVideoCodec_ == nullptr)
			throw ffmpeg_error(ffmpeg_make_error_code(ffmpeg_errc::decoder_not_found));
			
		std::error_code errc = ffmpeg_make_averror_code(avcodec_open((*videoStream)->codec, pVideoCodec_));
		if(errc)		
			throw ffmpeg_error(errc);

		succeeded = true;

		return std::shared_ptr<AVCodecContext>((*videoStream)->codec, avcodec_close);
	}

	std::shared_ptr<AVCodecContext> OpenAudioStream()
	{	
		bool succeeded = false;

		CASPAR_SCOPE_EXIT([&]
		{
			if(!succeeded)
			{
				audioStreamIndex_ = -1;
				pAudioCodec_ = nullptr;
			}
		});

		AVStream** streams_end = pFormatContext_->streams+pFormatContext_->nb_streams;
		AVStream** audioStream = std::find_if(pFormatContext_->streams, streams_end, 
			[](AVStream* stream) { return stream != nullptr && stream->codec->codec_type == CODEC_TYPE_AUDIO;});

		audioStreamIndex_ = audioStream != streams_end ? (*audioStream)->index : -1;
		if(audioStreamIndex_ == -1)
			return nullptr;
		
		pAudioCodec_ = avcodec_find_decoder((*audioStream)->codec->codec_id);
		if(pAudioCodec_ == 0)
			throw ffmpeg_error(ffmpeg_make_error_code(ffmpeg_errc::decoder_not_found));
		
		std::error_code errc = ffmpeg_make_averror_code(avcodec_open((*audioStream)->codec, pAudioCodec_));
		if(errc)		
			throw ffmpeg_error(errc);		

		succeeded = true;

		return std::shared_ptr<AVCodecContext>((*audioStream)->codec, avcodec_close);
	}	

	void* operator()(void*)
	{
		assert(pFrameManager_); // NOTE: FrameManager must be initialized before filter is run

		while(isRunning_)
		{
			AVPacket packet;
			std::shared_ptr<AVPacket> pPacket(&packet, av_free_packet);

			if (av_read_frame(pFormatContext_.get(), pPacket.get()) >= 0 && isRunning_) // NOTE: Packet is only valid until next call of av_read_frame or av_close_input_file
			{
				if(pPacket->stream_index == videoStreamIndex_) 				
				{
					tbb::spin_mutex::scoped_lock lock(mutex_);
					return new FFMPEGVideoPacket(pPacket, pFrameManager_->CreateFrame(), pFrameManager_->GetFrameFormatDescription(), // NOTE: FFMPEGVideoPacket makes a copy of AVPacket
												 pVideoCodecContext_.get(), pVideoCodec_, audioStreamIndex_ != -1);		
				}
				else if(pPacket->stream_index == audioStreamIndex_) 	
				{
					return new FFMPEGAudioPacket(pPacket, pAudioCodecContext_.get(), pAudioCodec_, videoFrameRate_);			
				}
				else
					assert(false && "Unknown packet type");
			}
			else if(!loop_ || av_seek_frame(pFormatContext_.get(), -1, 0, AVSEEK_FLAG_BACKWARD) < 0) // TODO: av_seek_frame does not work for all formats
			{
				isRunning_ = false;
			}
		}

		return nullptr;
	}
	
	void Stop()
	{
		isRunning_ = false;
	}
	
	void SetFactory(const FrameManagerPtr& pFrameManger)
	{
		tbb::spin_mutex::scoped_lock lock(mutex_);
		pFrameManager_ = pFrameManger;
	}
	
	double videoFrameRate_;
	
	std::shared_ptr<AVFormatContext>	pFormatContext_;	// Destroy this last

	std::shared_ptr<AVCodecContext>		pVideoCodecContext_;
	AVCodec*							pVideoCodec_;

	std::shared_ptr<AVCodecContext>		pAudioCodecContext_;
	AVCodec*							pAudioCodec_;

	tbb::atomic<bool>					isRunning_;
	tbb::atomic<bool>					loop_;
	int									videoStreamIndex_;
	int									audioStreamIndex_;
	FrameManagerPtr						pFrameManager_;
	tbb::spin_mutex						mutex_;
};

InputFilter::InputFilter() : tbb::filter(serial_in_order), pImpl_(new Implementation())
{
}

void InputFilter::Load(const std::string& filename, std::error_code& errc)
{
	pImpl_->Load(filename, errc);
}

void InputFilter::SetFactory(const FrameManagerPtr& pFrameManger)
{
	pImpl_->SetFactory(pFrameManger);
}

void* InputFilter::operator()(void*)
{
	return (*pImpl_)(NULL);
}

void InputFilter::SetLoop(bool value)
{
	pImpl_->loop_ = value;
}

void InputFilter::Stop()
{
	pImpl_->Stop();
}

const FrameManagerPtr InputFilter::GetFrameManager() const
{
	return pImpl_->pFrameManager_;
}

const std::shared_ptr<AVCodecContext>& InputFilter::VideoCodecContext() const
{
	return pImpl_->pVideoCodecContext_;
}

const std::shared_ptr<AVCodecContext>& InputFilter::AudioCodecContext() const
{
	return pImpl_->pAudioCodecContext_;
}
	}
}