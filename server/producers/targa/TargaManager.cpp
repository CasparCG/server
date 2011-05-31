#include "..\..\stdafx.h"

#include "TargaManager.h"
#include "..\..\frame\FrameManager.h"
#include "..\..\frame\FrameMediaController.h"
#include "..\..\utils\FileInputStream.h"
#include "..\..\fileinfo.h"

namespace caspar {

using namespace caspar::utils;

///////////////////////////////
//  TargaProducer declaration
//
class TargaProducer : public MediaProducer, public FrameMediaController
{
public:
	explicit TargaProducer(PixmapDataPtr pImage);
	virtual ~TargaProducer();

	virtual IMediaController* QueryController(const tstring& id);

	virtual bool Initialize(FrameManagerPtr pFrameManager);
	virtual FrameBuffer& GetFrameBuffer() {
		return frameBuffer_;
	}

private:
	StaticFrameBuffer frameBuffer_;
	caspar::utils::PixmapDataPtr pImage_;
};

//////////////////////////////
//  TargaManager definition
//
TargaManager::TargaManager()
{
	_extensions.push_back(TEXT("tga"));
}

TargaManager::~TargaManager()
{}

MediaProducerPtr TargaManager::CreateProducer(const tstring& filename) {
	MediaProducerPtr result;
	if(filename.length() > 0) {
		utils::InputStreamPtr pTgaFile(new utils::FileInputStream(filename));
		if(pTgaFile->Open()) {
			PixmapDataPtr pImage;
			if(TargaManager::Load(pTgaFile, pImage)) {
				result = MediaProducerPtr(new TargaProducer(pImage));
			}
		}
	}
	return result;
}

bool TargaManager::getFileInfo(FileInfo* pFileInfo)
{
	if(pFileInfo != 0) {
		pFileInfo->length = 1;
		pFileInfo->type = TEXT("still");
		pFileInfo->encoding = TEXT("TGA");
		return true;
	}
	return false;
}

PixmapDataPtr TargaManager::CropPadToFrameFormat(PixmapDataPtr pSource, const FrameFormatDescription& fmtDesc)
{
	if(pSource->width == fmtDesc.width && pSource->height == fmtDesc.height)
		return pSource;

	unsigned short colsToCopy = pSource->width;
	unsigned short rowsToCopy = pSource->height;

	int offsetX = 0;
	if(pSource->width > fmtDesc.width)
	{
		offsetX = (pSource->width-fmtDesc.width)/2;
		colsToCopy = fmtDesc.width;
	}

	int offsetY = 0;
	if(pSource->height > fmtDesc.height)
	{
		offsetY = (pSource->height-fmtDesc.height)/2;
		rowsToCopy = fmtDesc.height;
	}

	int bytesPerPixel = pSource->bpp;

	PixmapDataPtr pNewImage(new caspar::utils::PixmapData(fmtDesc.width, fmtDesc.height, bytesPerPixel));
	unsigned char* pNewImageData = pNewImage->GetDataPtr();

	//initialize new buffer with zeroes
	memset(pNewImageData, 0, fmtDesc.width*fmtDesc.height*bytesPerPixel);

	for(int i=0;i<rowsToCopy;++i)
		memcpy(&(pNewImageData[bytesPerPixel*fmtDesc.width*i]), &(pSource->GetDataPtr()[bytesPerPixel*(pSource->width*(i+offsetY)+offsetX)]), bytesPerPixel*colsToCopy);

	return pNewImage;
}


bool TargaManager::Load(utils::InputStreamPtr spTGA, PixmapDataPtr& pResult)
{
	utils::InputStream* pTGA = spTGA.get();
	PixmapDataPtr pImage(new utils::PixmapData());

	bool returnValue = true;
	//correct headers to compare to
	char headerUncompressed[12]	= {0,0, 2,0,0,0,0,0,0,0,0,0};
	char headerCompressed[12]	= {0,0,10,0,0,0,0,0,0,0,0,0};

	unsigned char header[12];
	unsigned char tgaInfo[6];
	unsigned int nImageSize = 0, nBytesPerPixel = 0;

	unsigned char *pColorBuffer = NULL;

	unsigned int i;
	unsigned int nBytesPerRow;
	int nCurrentRow = 0;

	char rowDirection = -1;

	pTGA->Read(header, 12); //read header to be able to process compressed files in one way and uncompressed files in another
	pTGA->Read(tgaInfo, 6); //read image-info such as height and width

	int width = tgaInfo[1] * 256 + tgaInfo[0];	//extract width
	int height = tgaInfo[3] * 256 + tgaInfo[2];	//extract height
	int bits = tgaInfo[4];						//extract bits/pixel

	pImage->Set(width, height, 4);
	unsigned char* pImageData = pImage->GetDataPtr();

	rowDirection = -1;
	if(tgaInfo[5] & 0x20)
		rowDirection = 1;

	//calculate usefull numbers
	nBytesPerPixel = bits / 8;
	nBytesPerRow = pImage->width * nBytesPerPixel;

	//internal data always have 4 bytes / pixel
	nImageSize = pImage->width * pImage->height * 4;

	if(nBytesPerPixel != 3 && nBytesPerPixel != 4)
	{
		returnValue = false;
		goto tgaLoaderExit;
	}

	memset(pImageData, 0, nImageSize);

	//Workaround for image identification field problem
	{
		unsigned char iifSize = header[0];
		unsigned char temp[256];
		if(iifSize > 0)
			pTGA->Read(temp, iifSize);
	}
	header[0] = 0;
	header[7] = 0;
	//END workaround

	//We've got an uncompressed file on our hands, take care of it
	if(memcmp(headerUncompressed, header, 12) == 0)
	{
		unsigned char* pRowBuffer = new unsigned char[nBytesPerRow];

		int rowIndex=0;
		if(rowDirection == -1)
			nCurrentRow = height-1;
		else
			nCurrentRow = 0;

		for(; rowIndex < height; nCurrentRow+=rowDirection, ++rowIndex)
		{
			if(pTGA->Read(pRowBuffer, nBytesPerRow) < nBytesPerRow)
			{
				delete[] pRowBuffer;
				returnValue = false;
				goto tgaLoaderExit;
			}
 
			//Swap color-channels
			for(i=0;i<pImage->width;i++)
			{
				pImageData[4*(nCurrentRow*width+i)+0] = pRowBuffer[i*nBytesPerPixel+0];
				pImageData[4*(nCurrentRow*width+i)+1] = pRowBuffer[i*nBytesPerPixel+1];
				pImageData[4*(nCurrentRow*width+i)+2] = pRowBuffer[i*nBytesPerPixel+2];
				if(nBytesPerPixel == 4)
					pImageData[4*(nCurrentRow*width+i)+3] = pRowBuffer[i*nBytesPerPixel+3];
				else
					pImageData[4*(nCurrentRow*width+i)+3] = 255;
			}
		}
		delete[] pRowBuffer;
	}

	//wasn't uncompressed, is it compressed? in that case, take care of it!
	else if(memcmp(headerCompressed, header, 12) == 0)
	{
		int rowIndex=0;
		if(rowDirection == -1)
			nCurrentRow = pImage->height-1;
		else
			nCurrentRow = 0;

		nBytesPerRow = width * 4;

		int nPixelCount = height * width;
		int nCurrentPixel = 0;
		int nCurrentByte = 0;

		pColorBuffer = new unsigned char[nBytesPerPixel];

		do
		{
			unsigned char chunkHeader = 0;

			//read chunkHeader - exit on error
			if(pTGA->Read(&chunkHeader, 1) < 0)
			{
				returnValue = false;
				goto tgaLoaderExit;
			}
			
			if(chunkHeader < 128) //it's a RAW-header
			{
				chunkHeader++;
				
				for(unsigned short counter=0; counter<chunkHeader; counter++)
				{
					//read pixeldata - exit on error
					if(pTGA->Read(pColorBuffer, nBytesPerPixel) < nBytesPerPixel)
					{
						returnValue = false;
						goto tgaLoaderExit;
					}

					unsigned int thisByte = nCurrentRow*nBytesPerRow+nCurrentByte;


					pImageData[thisByte+0] = pColorBuffer[0];
					pImageData[thisByte+1] = pColorBuffer[1];
					pImageData[thisByte+2] = pColorBuffer[2];
					if(nBytesPerPixel == 4)
						pImageData[thisByte+3] = pColorBuffer[3];
					else
						pImageData[thisByte+3] = 255;

					nCurrentByte += 4;
					nCurrentPixel++;
					if(nCurrentByte >= nBytesPerRow)
					{
						nCurrentRow +=  rowDirection;
						++rowIndex;
						nCurrentByte = 0;
					}
				}
			}
			else //it's a RLE header
			{
				chunkHeader -= 127;

				//read pixeldata - exit on error
				if(pTGA->Read(pColorBuffer, nBytesPerPixel) < nBytesPerPixel)
				{
					returnValue = false;
					goto tgaLoaderExit;
				}

				//repeat this pixel
				for(unsigned short counter=0; counter<chunkHeader; counter++)
				{
					unsigned int thisByte = nCurrentRow*nBytesPerRow+nCurrentByte;


					pImageData[thisByte+0] = pColorBuffer[0];
					pImageData[thisByte+1] = pColorBuffer[1];
					pImageData[thisByte+2] = pColorBuffer[2];
					if(nBytesPerPixel == 4)
						pImageData[thisByte+3] = pColorBuffer[3];
					else
						pImageData[thisByte+3] = 255;

					nCurrentByte += 4;
					nCurrentPixel++;
					if(nCurrentByte >= nBytesPerRow)
					{
						nCurrentRow += rowDirection;
						++rowIndex;
						nCurrentByte = 0;
					}
				}
			}
		}while(nCurrentPixel<nPixelCount && rowIndex < pImage->height);
	}
	else
		returnValue = false;

tgaLoaderExit:
	if(pColorBuffer != NULL)
		delete[] pColorBuffer;

	if(returnValue)
		pResult = pImage;

	return returnValue;
}


///////////////////////////////
//  TargaProducer definition
//
TargaProducer::TargaProducer(PixmapDataPtr pImage) : pImage_(pImage) {
}

TargaProducer::~TargaProducer() {
}

IMediaController* TargaProducer::QueryController(const tstring& id) {
	if(id == TEXT("FrameController"))
		return this;
	
	return 0;
}

bool TargaProducer::Initialize(FrameManagerPtr pFrameManager) {
	if(pFrameManager != 0) {
		FramePtr pFrame = pFrameManager->CreateFrame();
		if(pFrame != 0 && pFrame->GetDataPtr() != 0) {
			PixmapDataPtr pResult = TargaManager::CropPadToFrameFormat(pImage_, pFrameManager->GetFrameFormatDescription());

			unsigned char* pFrameData = pFrame->GetDataPtr();
			unsigned char* pImageData = pResult->GetDataPtr();

			memcpy(pFrameData, pImageData, pFrame->GetDataSize());

			frameBuffer_.push_back(pFrame);
			return true;
		}
	}
	return false;
}

}	//namespace caspar