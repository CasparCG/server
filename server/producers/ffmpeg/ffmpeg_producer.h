#pragma once

#include <string>
#include <vector>

#include "../../MediaProducer.h"
#include "../../MediaManager.h"
#include "../../MediaProducerInfo.h"

namespace caspar { namespace ffmpeg {
	
class FFMPEGProducer : public MediaProducer
{
public:
	FFMPEGProducer(const tstring& filename);
	
	IMediaController* QueryController(const tstring&);
	bool GetProducerInfo(MediaProducerInfo* pInfo);

	void SetLoop(bool loop);

	static const size_t MAX_TOKENS = 5;
	static const size_t MIN_BUFFER_SIZE = 2;
	static const size_t DEFAULT_BUFFER_SIZE = 16;
	static const size_t MAX_BUFFER_SIZE = 64;
	static const size_t LOAD_TARGET_BUFFER_SIZE = 8;
	static const size_t THREAD_TIMEOUT_MS = 1000;

private:	
	struct Implementation;
	std::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<FFMPEGProducer> FFMPEGProducerPtr;
}}