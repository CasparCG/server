/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
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

enum Stretch
{
	None,
	Uniform,
	Fill,
	UniformToFill
};

class OGLVideoConsumer : public IVideoConsumer, private utils::LockableObject
{
public:

	OGLVideoConsumer(HWND hWnd, const FrameFormatDescription& fmtDesc, unsigned int screenIndex = 0, Stretch stretch = Fill);
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