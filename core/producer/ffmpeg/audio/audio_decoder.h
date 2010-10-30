#pragma once

#include "../packet.h"

namespace caspar { namespace core { namespace ffmpeg	{

class audio_decoder : boost::noncopyable
{
public:
	audio_decoder();
	audio_packet_ptr execute(const audio_packet_ptr& audio_packet);
	
	/// <summary> The alignment </summary>
	/// <remarks> Four sec of 16 bit stereo 48kHz should be enough </remarks>
	static const int ALIGNMENT = 16 ;

	/// <summary> Size of the audio decomp buffer </summary>
	static const int AUDIO_DECOMP_BUFFER_SIZE = 4*48000*4+ALIGNMENT;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<audio_decoder> audio_decoder_ptr;
typedef std::unique_ptr<audio_decoder> audio_decoder_uptr;

}}}