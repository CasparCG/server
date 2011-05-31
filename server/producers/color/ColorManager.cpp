#include "..\..\stdafx.h"

#include "ColorManager.h"
#include "..\..\frame\FrameManager.h"
#include "..\..\frame\FrameMediaController.h"
#include "..\..\utils\FileInputStream.h"
#include "..\..\FileInfo.h"

#include <intrin.h>
#pragma intrinsic(__movsd, __stosd)
#pragma intrinsic(__movsw, __stosw)

void memset_w(unsigned short* pBuffer, unsigned short value, std::size_t count)
{
	__stosw(pBuffer, value, count);
}

void memset_d(unsigned long* pBuffer, unsigned long value, std::size_t count)
{
	__stosd(pBuffer, value, count);
}

namespace caspar {

///////////////////////////////
//  ColorProducer declaration
//
class ColorProducer : public MediaProducer, FrameMediaController
{
public:
	explicit ColorProducer(unsigned long colorValue);
	virtual ~ColorProducer();

	virtual IMediaController* QueryController(const tstring& id);

	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer() {
		return frameBuffer_;
	}

	virtual bool IsEmpty() const
	{
		return colorValue_ == 0;
	}

private:
	StaticFrameBuffer frameBuffer_;
	unsigned long colorValue_;
};

union Color {
	struct Components {
		unsigned char a;
		unsigned char r;
		unsigned char g;
		unsigned char b;
	} comp;

	unsigned long value;
};

//////////////////////////////
//  ColorManager definition
//
MediaProducerPtr ColorManager::CreateProducer(const tstring& parameter)
{
	MediaProducerPtr result;
	if(parameter[0] == '#') {
		unsigned long value;
		if(GetPixelColorValueFromString(parameter, &value))
			result = MediaProducerPtr(new ColorProducer(value));
	}

	return result;
}

bool ColorManager::getFileInfo(FileInfo* pFileInfo)
{
	if(pFileInfo != 0) {
		pFileInfo->length = 1;
		pFileInfo->type = TEXT("still");
		pFileInfo->encoding = TEXT("NA");
		return true;
	}
	return false;
}

bool ColorManager::GetPixelColorValueFromString(const tstring& parameter, unsigned long* outValue)
{
	tstring colorCode;
	if(parameter.length() == 9 && parameter[0] == '#')
	{
		colorCode = parameter.substr(1);

		Color theCol;
		theCol.value = _tcstoul(colorCode.c_str(),0,16);
		unsigned char temp = theCol.comp.a;
		theCol.comp.a = theCol.comp.b;
		theCol.comp.b = temp;
		temp = theCol.comp.r;
		theCol.comp.r = theCol.comp.g;
		theCol.comp.g = temp;

		*outValue = theCol.value;
		return true;
	}
	return false;
}


///////////////////////////////
//  ColorProducer definition
//
ColorProducer::ColorProducer(unsigned long colorValue) : colorValue_(colorValue) {
}

ColorProducer::~ColorProducer() {
}

IMediaController* ColorProducer::QueryController(const tstring& id) {
	if(id == TEXT("FrameController"))
		return this;
	
	return 0;
}

bool ColorProducer::Initialize(FrameManagerPtr pFrameManager) {
	if(pFrameManager != 0) {
		FramePtr pFrame = pFrameManager->CreateFrame();
		if(pFrame != 0)	{
			memset_d(reinterpret_cast<unsigned long*>(pFrame->GetDataPtr()), colorValue_, pFrame->GetDataSize() / sizeof(unsigned long));
			frameBuffer_.push_back(pFrame);
			return true;
		}
	}
	return false;
}

}