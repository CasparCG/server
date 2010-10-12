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
 
#ifndef TARGASCROLLMediaProducer_H
#define TARGASCROLLMediaProducer_H

#include "..\..\MediaProducer.h"
#include "..\..\utils\Thread.h"
#include "..\..\utils\Lockable.h"
#include "..\..\frame\Framemediacontroller.h"
#include "..\..\frame\frame.h"

namespace caspar {
namespace utils {
	class PixmapData;
	typedef std::tr1::shared_ptr<PixmapData> PixmapDataPtr;
}

class TargaScrollMediaProducer : public MediaProducer, public FrameMediaController, public utils::IRunnable, utils::LockableObject
{
	static int DEFAULT_SPEED;
public:
	explicit TargaScrollMediaProducer();
	TargaScrollMediaProducer(const TargaScrollMediaProducer&);
	virtual ~TargaScrollMediaProducer();

	bool Load(const tstring& filename);

	virtual IMediaController* QueryController(const tstring& id);
	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer() {
		return frameBuffer;
	}

	virtual void Run(HANDLE stopEvent);
	virtual bool OnUnhandledException(const std::exception& ex) throw();

private:
	void PadImageToFrameFormat();
	FramePtr FillVideoFrame(FramePtr pFrame);

	struct DirectionFlag
	{
		enum DirectionFlagEnum
		{
			ScrollUp = 1,
			ScrollDown,
			ScrollLeft,
			ScrollRight
		};
	};

	int offset;
	short speed;

	utils::Thread workerThread;
	utils::PixmapDataPtr pImage;

	MotionFrameBuffer frameBuffer;
	DirectionFlag::DirectionFlagEnum direction;

	utils::Event initializeEvent_;
	FrameManagerPtr pFrameManager_;
	FrameManagerPtr pTempFrameManager_;
};

}

#endif