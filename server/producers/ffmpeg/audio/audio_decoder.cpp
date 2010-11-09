#include "../../../stdafx.h"

#include "audio_decoder.h"

#include <queue>
		
namespace caspar{ namespace ffmpeg{

struct audio_decoder::implementation : boost::noncopyable
{
	implementation() : discard_bytes_(0), current_audio_chunk_offset_(0)
	{
		audio_decomp_buffer_.resize(audio_decoder::AUDIO_DECOMP_BUFFER_SIZE);
		int alignment_offset_ = static_cast<unsigned char>(audio_decoder::ALIGNMENT - (reinterpret_cast<size_t>(&audio_decomp_buffer_.front()) % audio_decoder::ALIGNMENT));
		aligned_audio_decomp_addr_ = &audio_decomp_buffer_.front() + alignment_offset_;				
	}
		
	audio_packet_ptr execute(const audio_packet_ptr& audio_packet)
	{			
		int max_chunk_length = min(audio_packet->audio_frame_size, audio_packet->src_audio_frame_size);

		int written_bytes = audio_decoder::AUDIO_DECOMP_BUFFER_SIZE - audio_decoder::ALIGNMENT;
		int result = avcodec_decode_audio2(audio_packet->codec_context, reinterpret_cast<int16_t*>(aligned_audio_decomp_addr_), &written_bytes, audio_packet->data.data(), audio_packet->size);

		if(result <= 0)
			return audio_packet;

		unsigned char* pDecomp = aligned_audio_decomp_addr_;

		//if there are bytes to discard, do that first
		while(written_bytes > 0 && discard_bytes_ != 0)
		{
			int bytesToDiscard = min(written_bytes, static_cast<int>(discard_bytes_));
			pDecomp += bytesToDiscard;

			discard_bytes_ -= bytesToDiscard;
			written_bytes -= bytesToDiscard;
		}

		while(written_bytes > 0)
		{
			//if we're starting on a new chunk, allocate it
			if(current_chunk_ == nullptr) 
			{
				current_chunk_ = std::make_shared<audio::AudioDataChunk>(audio_packet->audio_frame_size);
				current_audio_chunk_offset_ = 0;
			}

			//either fill what's left of the chunk or copy all written_bytes that are left
			int targetLength = min((max_chunk_length - current_audio_chunk_offset_), written_bytes);
			memcpy(current_chunk_->GetDataPtr() + current_audio_chunk_offset_, pDecomp, targetLength);
			written_bytes -= targetLength;

			current_audio_chunk_offset_ += targetLength;
			pDecomp += targetLength;

			if(current_audio_chunk_offset_ >= max_chunk_length) 
			{
				if(max_chunk_length < static_cast<int>(audio_packet->audio_frame_size)) 
					memset(current_chunk_->GetDataPtr() + max_chunk_length, audio_packet->audio_frame_size-max_chunk_length, 0);					
				else if(audio_packet->audio_frame_size < audio_packet->src_audio_frame_size) 
					discard_bytes_ = audio_packet->src_audio_frame_size-audio_packet->audio_frame_size;

				audio_packet->audio_chunks.push_back(current_chunk_);
				current_chunk_.reset();
			}
		}

		return audio_packet;
	}
			
	int									discard_bytes_;
		
	std::vector<unsigned char>			audio_decomp_buffer_;
	unsigned char*						aligned_audio_decomp_addr_;

	audio::AudioDataChunkPtr						current_chunk_;
	int									current_audio_chunk_offset_;
};

audio_decoder::audio_decoder() : impl_(new implementation()){}
audio_packet_ptr audio_decoder::execute(const audio_packet_ptr& audio_packet){return impl_->execute(audio_packet);}
}}