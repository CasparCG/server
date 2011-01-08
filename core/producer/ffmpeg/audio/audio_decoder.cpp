#include "../../../stdafx.h"

#include "audio_decoder.h"

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavformat/avformat.h>
	#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar { namespace core { namespace ffmpeg{

struct audio_decoder::implementation : boost::noncopyable
{
	static const int FRAME_AUDIO_SAMPLES = 1920*2;
	static const int SAMPLE_RATE = 48000;

	implementation(AVCodecContext* codec_context) : current_chunk_(), codec_context_(codec_context), audio_buffer_(4*SAMPLE_RATE*2+FF_INPUT_BUFFER_PADDING_SIZE/2){}
		
	std::vector<std::vector<short>> execute(const aligned_buffer& audio_packet)
	{			
		static const std::vector<std::vector<short>> silence(1920*2, 0);
		
		int written_bytes = audio_buffer_.size()*2 - FF_INPUT_BUFFER_PADDING_SIZE;
		const int result = avcodec_decode_audio2(codec_context_, audio_buffer_.data(), &written_bytes, audio_packet.data(), audio_packet.size());

		if(result <= 0 || codec_context_->sample_rate != SAMPLE_RATE)
			return silence;
						
		current_chunk_.insert(current_chunk_.end(), audio_buffer_.data(), audio_buffer_.data() + written_bytes/2);

		std::vector<std::vector<short>> chunks_;
				
		const auto last = current_chunk_.end() - current_chunk_.size() % FRAME_AUDIO_SAMPLES;

		for(auto it = current_chunk_.begin(); it != last; it += FRAME_AUDIO_SAMPLES)		
			chunks_.push_back(std::vector<short>(it, it + FRAME_AUDIO_SAMPLES));		

		current_chunk_.erase(current_chunk_.begin(), last);
		
		return chunks_;
	}

	typedef std::vector<short, tbb::cache_aligned_allocator<short>> buffer;

	buffer audio_buffer_;	

	std::vector<short, tbb::cache_aligned_allocator<short>> current_chunk_;

	AVCodecContext*	codec_context_;
};

audio_decoder::audio_decoder(AVCodecContext* codec_context) : impl_(new implementation(codec_context)){}
std::vector<std::vector<short>> audio_decoder::execute(const aligned_buffer& audio_packet){return impl_->execute(audio_packet);}
}}}