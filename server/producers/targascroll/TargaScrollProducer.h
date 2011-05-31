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