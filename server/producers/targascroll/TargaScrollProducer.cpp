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
 
#include "..\..\StdAfx.h"

#include "TargaScrollProducer.h"
#include "..\Targa\TargaManager.h"
#include "..\..\MediaProducer.h"
#include "..\..\FileInfo.h"
#include "..\..\utils\FileInputStream.h"
#include "..\..\utils\PixmapData.h"

namespace caspar {
using namespace utils;

TargaScrollMediaProducer::TargaScrollMediaProducer(FrameFormat format) : MediaProducer(format)
{
}

TargaScrollMediaProducer::~TargaScrollMediaProducer()
{
	this->workerThread.Stop();
}

bool TargaScrollMediaProducer::Load(const tstring& filename)
{
	if (filename.length() > 0)
	{
		utils::InputStreamPtr pTgaFile(new utils::FileInputStream(filename));
		if (pTgaFile->Open())
			return TargaMediaManager::Load(pTgaFile, this->pImage);
	}

	return false;
}

bool TargaScrollMediaProducer::DoInitialize()
{
	this->workerThread.Start(this);
	return true;
}

FrameBuffer& TargaScrollMediaProducer::GetFrameBuffer()
{
	return this->frameBuffer;
}

void TargaScrollMediaProducer::PadImageToFrameFormat()
{
	const FrameFormatDescription& formatDescription = FrameFormatDescription::FormatDescriptions[GetFormat()];

	const unsigned int PIXMAP_WIDTH = max(this->pImage->width, formatDescription.width);
	const unsigned int PIXMAP_HEIGHT = max(this->pImage->height, formatDescription.height);

	utils::PixmapDataPtr pNewImage(new utils::PixmapData(PIXMAP_WIDTH, PIXMAP_HEIGHT, this->pImage->bpp));

	unsigned char* pNewImageData = pNewImage->GetDataPtr();
	unsigned char* pImageData = this->pImage->GetDataPtr();

	memset(pNewImageData, 0, pNewImage->width * pNewImage->height * pNewImage->bpp);

	for (int i = 0; i < this->pImage->height; ++i)
		memcpy(&pNewImageData[i* pNewImage->width * pNewImage->bpp], &pImageData[i* this->pImage->width * this->pImage->bpp], this->pImage->width * this->pImage->bpp);

	this->pImage = pNewImage;
}

FramePtr TargaScrollMediaProducer::FillVideoFrame(FramePtr pFrame)
{
	const FrameFormatDescription& formatDescription = FrameFormatDescription::FormatDescriptions[GetFormat()];

	const short deltaX = this->direction == DirectionFlag::ScrollLeft ? this->speed : -this->speed;
	const short deltaY = this->direction == DirectionFlag::ScrollUp ? this->speed : -this->speed;

	unsigned char* pFrameData = pFrame->GetDataPtr();
	unsigned char* pImageData = this->pImage->GetDataPtr();
	
	bool isFirstFrame = false, isLastFrame = false;
	if (this->direction == DirectionFlag::ScrollUp || this->direction == DirectionFlag::ScrollDown)
	{
		for (int i = 0; i < formatDescription.height; ++i)
		{
			int srcRow = i + this->offset; // Assume progressive.
			if (formatDescription.mode == Interlaced)
			{
				const int nextOffset = this->offset + (deltaY * 2);
				isFirstFrame = this->offset == 0 || this->offset == (this->pImage->height - formatDescription.height);
				isLastFrame = nextOffset <= 0 || nextOffset >= (this->pImage->height - formatDescription.height);
				if (!isFirstFrame && !isLastFrame)
					srcRow = (i % 2 == 0) ? i + this->offset : min(i + this->offset + deltaY, this->pImage->height);
			}

			int dstInxex = i * formatDescription.width * this->pImage->bpp;
			int srcIndex = srcRow * formatDescription.width * this->pImage->bpp;
			int size = formatDescription.width * this->pImage->bpp;

			memcpy(&pFrameData[dstInxex], &pImageData[srcIndex], size);	
		}

		if (formatDescription.mode == Interlaced && !isFirstFrame && !isLastFrame)
			this->offset += deltaY * 2;
		else
			this->offset += deltaY;
	}
	else
	{
		for (int i = 0; i < formatDescription.height; ++i)
		{
			int correctOffset = this->offset; // Assume progressive.
			if (formatDescription.mode == Interlaced)
			{
				const int nextOffset = this->offset + (deltaX * 2); // Next offset.
				isFirstFrame = this->offset == 0 || this->offset == (this->pImage->width - formatDescription.width);
				isLastFrame = nextOffset <= 0 || nextOffset >= (this->pImage->width - formatDescription.width);
				if (!isFirstFrame && !isLastFrame)
					correctOffset = (i % 2 == 0) ? this->offset: this->offset + deltaX;
			}

			int dstIndex = i * formatDescription.width * this->pImage->bpp;
			int srcIndex = (i * this->pImage->width + correctOffset) * this->pImage->bpp;
			
			int stopOffset = min(correctOffset + formatDescription .width, this->pImage->width);
			int size = (stopOffset - correctOffset) * this->pImage->bpp;

			memcpy(&pFrameData[dstIndex], &pImageData[srcIndex], size);
		}

		if (formatDescription.mode == Interlaced && !isFirstFrame && !isLastFrame)
			this->offset += deltaX * 2;
		else
			this->offset += deltaX;
	}

	return pFrame;
}

void TargaScrollMediaProducer::Run(HANDLE stopEvent)
{
	LOG << LogLevel::Verbose << TEXT("Targa scroll thread started") << LogStream::Flush;

	const short waitHandleCount = 2;
	HANDLE waitHandles[waitHandleCount] = { stopEvent, this->frameBuffer.GetWriteWaitHandle() };

	const FrameFormatDescription& formatDescription = FrameFormatDescription::FormatDescriptions[GetFormat()];



	// OBS! This will be setup by the tga loader.
	this->speed = 5;
	this->direction = DirectionFlag::ScrollDown;

	this->offset = 0;
	if (this->direction == DirectionFlag::ScrollDown)
		this->offset = this->pImage->height - formatDescription.height;
	else if (this->direction == DirectionFlag::ScrollRight)
		this->offset = this->pImage->width - formatDescription.width;



	if (formatDescription.width > this->pImage->width || formatDescription.height > this->pImage->height)
		PadImageToFrameFormat();

	bool quitLoop = false;
	while (!quitLoop)
	{
		HRESULT waitResult = WaitForMultipleObjects(waitHandleCount, waitHandles, FALSE, 1000);
		switch(waitResult)
		{
			case WAIT_OBJECT_0 + 0:		// Stop.
			case WAIT_FAILED:			// Wait failiure.
				quitLoop = true;
				continue;
			case WAIT_TIMEOUT:			// Nothing has happened.
				continue;
			case WAIT_OBJECT_0 + 1:		// Framebuffer is ready to be filled.
			{
				// Render next frame.
				FramePtr pFrame = GetFactory()->CreateFrame();
				pFrame = FillVideoFrame(pFrame);
				this->frameBuffer.push_back(pFrame);

				// Should we stop scrolling?
				if ((this->direction == DirectionFlag::ScrollDown || this->direction == DirectionFlag::ScrollRight) && this->offset <= 0)
					quitLoop = true;
				else if (this->direction == DirectionFlag::ScrollUp && this->offset >= (this->pImage->height - formatDescription.height))
					quitLoop = true;
				else if (this->direction == DirectionFlag::ScrollLeft && this->offset >= (this->pImage->width - formatDescription.width))
					quitLoop = true;
			}
		}
	}
	
	// Render a null frame to indicate EOF.
	FramePtr pNullFrame;
	this->frameBuffer.push_back(pNullFrame);

	LOG << LogLevel::Verbose << TEXT("Targa scroll thread ended") << LogStream::Flush;
}

bool TargaScrollMediaProducer::OnUnhandledException(const std::exception& ex) throw()
{
	try
	{
		FramePtr pNullFrame;
		this->frameBuffer.push_back(pNullFrame);

		LOG << LogLevel::Critical << TEXT("UNHANDLED EXCEPTION in targa scroll thread. Message: %hS") << ex.what() << LogStream::Flush;
	}
	catch (...)
	{
	}

	return false;
}

void TargaScrollMediaProducer::Param(const tstring&)
{
}

}