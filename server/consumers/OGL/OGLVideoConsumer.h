#ifndef _OGL_CONSUMER_H_
#define _OGL_CONSUMER_H_

#include "..\..\VideoConsumer.h"
#include "..\..\MediaProducer.h"
#include "..\..\frame\BitmapFrameManager.h"

#include "..\..\utils\thread.h"
#include "..\..\utils\lockable.h"

#include <memory>

namespace caspar {
namespace ogl {

class OGLVideoConsumer : public IVideoConsumer, private utils::LockableObject
{
public:

	OGLVideoConsumer(HWND hWnd, const FrameFormatDescription& fmtDesc, unsigned int screenIndex = 0, bool fullscreen = true, bool aspect = true);
	~OGLVideoConsumer(void);
	
	IPlaybackControl* GetPlaybackControl() const;
	void EnableVideoOutput();
	void DisableVideoOutput();
	bool SetupDevice(unsigned int deviceIndex);
	bool ReleaseDevice();
	const TCHAR* GetFormatDescription() const;
		
private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};

typedef std::tr1::shared_ptr<OGLVideoConsumer> OGLVideoConsumerPtr;


}
}

#endif