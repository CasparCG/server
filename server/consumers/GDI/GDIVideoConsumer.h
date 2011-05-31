#pragma once
#include "..\..\VideoConsumer.h"
#include "..\..\MediaProducer.h"

#include "..\..\utils\thread.h"
#include "..\..\utils\lockable.h"
#include "..\..\utils\Noncopyable.hpp"

#include "..\..\frame\BitmapFrameManager.h"
#include "..\..\frame\Frame.h"

namespace caspar {
namespace gdi {

class GDIVideoConsumer : public IVideoConsumer, private utils::LockableObject, utils::Noncopyable
{
public:
	GDIVideoConsumer(HWND hwnd, const FrameFormatDescription& fmtDesc);
	virtual ~GDIVideoConsumer();

	virtual IPlaybackControl* GetPlaybackControl() const;

	virtual void EnableVideoOutput();
	virtual void DisableVideoOutput();
	virtual bool SetupDevice(unsigned int deviceIndex);
	virtual bool ReleaseDevice();
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;
	virtual const TCHAR* GetFormatDescription() const;

private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};

}	//namespace gdi
}	//namespace caspar