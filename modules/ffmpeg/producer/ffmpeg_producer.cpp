#include "../stdafx.h"

#include "ffmpeg_producer.h"

#include "input.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include <common/utility/timer.h>
#include <common/diagnostics/graph.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/write_frame.h>
#include <core/producer/frame/audio_transform.h>
#include <core/video_format.h>

#include <common/env.h>
#include <common/utility/timer.h>
#include <common/utility/assert.h>

#include <tbb/parallel_invoke.h>

#include <deque>
#include <functional>

namespace caspar {
	
struct ffmpeg_producer : public core::frame_producer
{
	const std::wstring					filename_;
	const bool							loop_;
	
	std::shared_ptr<diagnostics::graph>	graph_;
	timer								perf_timer_;
		
	std::unique_ptr<audio_decoder>		audio_decoder_;
	std::unique_ptr<video_decoder>		video_decoder_;

	std::deque<safe_ptr<core::write_frame>>	video_frame_channel_;	
	std::deque<std::vector<short>>			audio_chunk_channel_;

	std::queue<safe_ptr<core::basic_frame>>	ouput_channel_;
		
	safe_ptr<core::basic_frame>				last_frame_;
	std::shared_ptr<core::frame_factory>	frame_factory_;

	std::unique_ptr<input>				input_;	
public:
	explicit ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::wstring& filename, bool loop) 
		: filename_(filename)
		, loop_(loop) 
		, last_frame_(core::basic_frame(core::basic_frame::empty()))
		, frame_factory_(frame_factory)		
	{
		graph_ = diagnostics::create_graph(boost::bind(&ffmpeg_producer::print, this));	
		graph_->add_guide("frame-time", 0.5);
		graph_->set_color("frame-time",  diagnostics::color(1.0f, 0.0f, 0.0f));

		input_.reset(new input(safe_ptr<diagnostics::graph>(graph_), filename_, loop_));
		video_decoder_.reset(input_->get_video_codec_context().get() ? new video_decoder(input_->get_video_codec_context().get(), frame_factory) : nullptr);
		audio_decoder_.reset(input_->get_audio_codec_context().get() ? new audio_decoder(input_->get_audio_codec_context().get(), frame_factory->get_video_format_desc().fps) : nullptr);

		double frame_time = 1.0f/input_->fps();
		double format_frame_time = 1.0/frame_factory->get_video_format_desc().fps;
		if(abs(frame_time - format_frame_time) > 0.0001)
			CASPAR_LOG(warning) << print() << L" Invalid framerate detected. This may cause distorted audio during playback. frame-time: " << frame_time;
	}
			
	virtual safe_ptr<core::basic_frame> receive()
	{
		perf_timer_.reset();

		while(ouput_channel_.size() < 2 && !input_->is_eof())
		{	
			aligned_buffer video_packet;
			if(video_frame_channel_.size() < 3 && video_decoder_)	
				video_packet = input_->get_video_packet();		
			
			aligned_buffer audio_packet;
			if(audio_chunk_channel_.size() < 3 && audio_decoder_)	
				audio_packet = input_->get_audio_packet();	

			if(video_packet.empty() && audio_packet.empty()) // Skip frame if lagging.			 
				break;

			tbb::parallel_invoke(
			[&]
			{
				if(!video_packet.empty() && video_decoder_) // Video Decoding.
				{
					try
					{
						auto frame = video_decoder_->execute(this, video_packet);
						video_frame_channel_.push_back(std::move(frame));
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						video_decoder_.reset();
						CASPAR_LOG(warning) << print() << " removed video-stream.";
					}
				}
			}, 
			[&] 
			{ 
				if(!audio_packet.empty() && audio_decoder_) // Audio Decoding.
				{
					try
					{
						auto chunks = audio_decoder_->execute(audio_packet);
						audio_chunk_channel_.insert(audio_chunk_channel_.end(), chunks.begin(), chunks.end());
					}
					catch(...)
					{
						CASPAR_LOG_CURRENT_EXCEPTION();
						audio_decoder_.reset();
						CASPAR_LOG(warning) << print() << " removed audio-stream.";
					}
				}
			});

			while((!video_frame_channel_.empty() || !video_decoder_) && (!audio_chunk_channel_.empty() || !audio_decoder_))
			{
				std::shared_ptr<core::write_frame> frame;

				if(video_decoder_)
				{
					frame = video_frame_channel_.front();
					video_frame_channel_.pop_front();
				}

				if(audio_decoder_) 
				{
					if(!frame) // If there is no video create a dummy frame.
					{
						frame = frame_factory_->create_frame(this, 1, 1);
						std::fill(frame->image_data().begin(), frame->image_data().end(), 0);
					}
					
					frame->audio_data() = std::move(audio_chunk_channel_.front());
					audio_chunk_channel_.pop_front();
				}
							
				ouput_channel_.push(safe_ptr<core::write_frame>(frame));				
			}				

			if(ouput_channel_.empty() && video_packet.empty() && audio_packet.empty())			
				return last_frame_;			
		}
		
		graph_->update_value("frame-time", static_cast<float>(perf_timer_.elapsed()/frame_factory_->get_video_format_desc().interval*0.5));

		auto result = last_frame_;
		if(!ouput_channel_.empty())		
			result = get_frame(); // TODO: Support 50p		
		else if(!input_->is_running())
			result = core::basic_frame::eof();
		else
			graph_->add_tag("lag");
		
		return result;
	}

	core::basic_frame get_frame()
	{
		CASPAR_ASSERT(!ouput_channel_.empty());
		auto result = std::move(ouput_channel_.front());
		last_frame_ = core::basic_frame(result);
		last_frame_->get_audio_transform().set_gain(0.0); // last_frame should not have audio
		ouput_channel_.pop();
		return result;
	}

	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
	}
};

safe_ptr<core::frame_producer> create_ffmpeg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{			
	static const std::vector<std::wstring> extensions = boost::assign::list_of
		(L"mpg")(L"mpeg")(L"avi")(L"mov")(L"qt")(L"webm")(L"dv")(L"mp4")(L"f4v")(L"flv")(L"mkv")(L"mka")(L"wmw")(L"wma")(L"ogg")(L"divx")(L"wav")(L"mp3");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return core::frame_producer::empty();

	std::wstring path = filename + L"." + *ext;
	bool loop = std::find(params.begin(), params.end(), L"LOOP") != params.end();
	
	return make_safe<ffmpeg_producer>(frame_factory, path, loop);
}

}