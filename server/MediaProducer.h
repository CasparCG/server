#pragma once

namespace caspar {

class IMediaController;

class MediaProducer;
typedef std::tr1::shared_ptr<MediaProducer> MediaProducerPtr;

class MediaProducer
{
	MediaProducer(const MediaProducer&);
	MediaProducer& operator=(const MediaProducer&);

public:
	MediaProducer() : bLoop_(false)
	{}
	virtual ~MediaProducer()
	{}

	virtual IMediaController* QueryController(const tstring&) = 0;

	virtual bool Param(const tstring&) { return false; }
	virtual bool IsEmpty() const { return false; }

	virtual MediaProducerPtr GetFollowingProducer() {
		return MediaProducerPtr();
	}

	void SetLoop(bool bLoop) {
		bLoop_ = bLoop;
	}
	bool GetLoop() {
		return bLoop_;
	}

private:
	bool bLoop_;
};

}	//namespace caspar