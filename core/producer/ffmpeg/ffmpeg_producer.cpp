#include "../../stdafx.h"

#include "ffmpeg_producer.h"

#include "input.h"
#include "audio/audio_decoder.h"
#include "video/video_decoder.h"

#include "../../video_format.h"
#include "../../mixer/frame/draw_frame.h"
#include "../../mixer/audio/audio_transform.h"

#include <common/env.h>

#include <tbb/parallel_invoke.h>

#include <deque>

namespace caspar { namespace core { namespace ffmpeg{
	
struct ffmpeg_producer : public frame_producer
{
	input								input_;			
	std::unique_ptr<audio_decoder>		audio_decoder_;
	video_decoder						video_decoder_;

	std::deque<safe_ptr<write_frame>>	video_frame_channel_;	
	std::deque<std::vector<short>>		audio_chunk_channel_;

	std::queue<safe_ptr<draw_frame>>	ouput_channel_;
	
	const std::wstring					filename_;
	
	safe_ptr<draw_frame>				last_frame_;

public:
	explicit ffmpeg_producer(const std::wstring& filename, bool loop) 
		: filename_(filename)
		, last_frame_(draw_frame(draw_frame::empty()))
		, input_(filename, loop)
		, video_decoder_(input_.get_video_codec_context().get())		
		, audio_decoder_(input_.get_audio_codec_context().get() ? new audio_decoder(input_.get_audio_codec_context().get(), input_.fps()) : nullptr){}

	virtual void initialize(const safe_ptr<frame_factory>& frame_factory)
	{
		video_decoder_.initialize(frame_factory);
	}
		
	virtual safe_ptr<draw_frame> receive()
	{
		while(ouput_channel_.empty() && !input_.is_eof())
		{	
			aligned_buffer video_packet;
			if(video_frame_channel_.size() < 3)	
				video_packet = input_.get_video_packet();		
			
			aligned_buffer audio_packet;
			if(audio_chunk_channel_.size() < 3)	
				audio_packet = input_.get_audio_packet();		

			tbb::parallel_invoke(
			[&]
			{ // Video Decoding and Scaling
				if(!video_packet.empty())
				{
					auto frame = video_decoder_.execute(video_packet);
					video_frame_channel_.push_back(std::move(frame));
				}
			}, 
			[&] 
			{ // Audio Decoding
				if(!audio_packet.empty() && audio_decoder_)
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
					}
				}
			});

			while(!video_frame_channel_.empty() && (!audio_chunk_channel_.empty() || !audio_decoder_))
			{
				if(audio_decoder_) 
				{
					video_frame_channel_.front()->audio_data() = std::move(audio_chunk_channel_.front());
					audio_chunk_channel_.pop_front();
				}
							
				ouput_channel_.push(std::move(video_frame_channel_.front()));
				video_frame_channel_.pop_front();
			}				

			if(ouput_channel_.empty() && video_packet.empty() && audio_packet.empty())			
				return last_frame_;			
		}

		auto result = last_frame_;
		if(!ouput_channel_.empty())
		{
			result = std::move(ouput_channel_.front());
			last_frame_ = draw_frame(result);
			last_frame_->get_audio_transform().set_gain(0.0); // last_frame should not have audio
			ouput_channel_.pop();
		}
		else if(input_.is_eof())
			return draw_frame::eof();

		return result;
	}

	virtual std::wstring print() const
	{
		return L"ffmpeg[" + boost::filesystem::wpath(filename_).filename() + L"]";
	}
};

safe_ptr<frame_producer> create_ffmpeg_producer(const std::vector<std::wstring>& params)
{			
	static const std::vector<std::wstring> extensions = boost::assign::list_of(L"mpg")(L"avi")(L"mov")(L"dv")(L"wav")(L"mp3")(L"mp4")(L"f4v")(L"flv");
	std::wstring filename = env::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return frame_producer::empty();

	std::wstring path = filename + L"." + *ext;
	bool loop = std::find(params.begin(), params.end(), L"LOOP") != params.end();
	
	return make_safe<ffmpeg_producer>(path, loop);
}

}}}