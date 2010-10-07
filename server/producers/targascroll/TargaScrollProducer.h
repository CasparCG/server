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

namespace caspar {
namespace utils {
	class PixmapData;
	typedef std::tr1::shared_ptr<PixmapData> PixmapDataPtr;
}

class TargaScrollMediaProducer : public MediaProducer, public utils::IRunnable
{
	public:
		explicit TargaScrollMediaProducer(FrameFormat format);
		TargaScrollMediaProducer(const TargaScrollMediaProducer&);
		virtual ~TargaScrollMediaProducer();

		bool Load(const tstring& filename);

		virtual void Run(HANDLE stopEvent);
		virtual void Param(const tstring&);
		virtual FrameBuffer& GetFrameBuffer();
		virtual bool DoInitialize();
		virtual bool OnUnhandledException(const std::exception& ex) throw();

	private:
		void PadImageToFrameFormat();
		FramePtr FillVideoFrame(FramePtr pFrame);

	private:
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
};

}

#endif