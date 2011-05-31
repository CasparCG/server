#pragma once

namespace caspar {

struct MediaProducerInfo {

	MediaProducerInfo() : HaveAudio(false), HaveVideo(false), AudioSamplesPerSec(0), AudioChannels(0), BitsPerAudioSample(0)
	{}

	bool HaveAudio;
	bool HaveVideo;

	unsigned int AudioSamplesPerSec;
	unsigned short AudioChannels;
	unsigned short BitsPerAudioSample;
};

}	//namespace caspar