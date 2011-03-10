#include "../../stdafx.h"

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

namespace caspar {

struct audio_decoder::implementation : boost::noncopyable
{
	typedef std::vector<short, tbb::cache_aligned_allocator<short>> buffer;
	
	AVCodecContext*	codec_context_;

	buffer audio_buffer_;	
	buffer current_chunk_;

	const size_t audio_frame_size_;

	static const size_t SAMPLE_RATE = 48000;
	static const size_t N_CHANNELS = 2;

public:
	explicit implementation(AVCodecContext* codec_context, double fps) 
		: codec_context_(codec_context)
		, audio_buffer_(4*SAMPLE_RATE*2+FF_INPUT_BUFFER_PADDING_SIZE/2)
		, audio_frame_size_(static_cast<size_t>(static_cast<double>(SAMPLE_RATE) / fps) * N_CHANNELS)
	{
		if(!codec_context)
			BOOST_THROW_EXCEPTION(null_argument() << arg_name_info("codec_context"));						
	}
		
	std::vector<std::vector<short>> execute(const aligned_buffer& audio_packet)
	{			
		int written_bytes = audio_buffer_.size()*2 - FF_INPUT_BUFFER_PADDING_SIZE;
		const int result = avcodec_decode_audio2(codec_context_, audio_buffer_.data(), &written_bytes, audio_packet.data(), audio_packet.size());

		if(result <= 0 || codec_context_->sample_rate != SAMPLE_RATE || codec_context_->channels != 2)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Invalid audio stream"));
						
		current_chunk_.insert(current_chunk_.end(), audio_buffer_.data(), audio_buffer_.data() + written_bytes/2);

		std::vector<std::vector<short>> chunks;
				
		const auto last = current_chunk_.end() - current_chunk_.size() % audio_frame_size_;

		for(auto it = current_chunk_.begin(); it != last; it += audio_frame_size_)		
			chunks.push_back(std::vector<short>(it, it + audio_frame_size_));		

		current_chunk_.erase(current_chunk_.begin(), last);
		
		return chunks;
	}
};

audio_decoder::audio_decoder(AVCodecContext* codec_context, double fps) : impl_(new implementation(codec_context, fps)){}
std::vector<std::vector<short>> audio_decoder::execute(const aligned_buffer& audio_packet){return impl_->execute(audio_packet);}
}