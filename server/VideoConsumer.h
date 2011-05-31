#ifndef __VIDEO_CONSUMER_H__
#define __VIDEO_CONSUMER_H__

namespace caspar {

class IPlaybackControl;
class MediaProducer;

class IVideoConsumer
{
public:
	virtual ~IVideoConsumer() {}
	virtual IPlaybackControl* GetPlaybackControl() const = 0;
	virtual void EnableVideoOutput() = 0;
	virtual void DisableVideoOutput() = 0;
	virtual bool SetupDevice(unsigned int deviceIndex) = 0;
	virtual bool ReleaseDevice() = 0;
	virtual const TCHAR* GetFormatDescription() const = 0;
};
typedef std::tr1::shared_ptr<IVideoConsumer> VideoConsumerPtr;

}	//namespace caspar
#endif //__VIDEO_CONSUMER_H__