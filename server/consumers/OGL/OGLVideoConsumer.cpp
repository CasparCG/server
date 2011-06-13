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
#include "OGLVideoConsumer.h"
#include "..\..\frame\FramePlaybackControl.h"
#include "..\..\frame\FramePlaybackStrategy.h"

#include <Glee.h>

namespace caspar {
namespace ogl {

struct OGLVideoConsumer::Implementation
{
	struct OGLDevice
	{
		OGLDevice(HWND hWnd, Stretch stretch, int screenWidth, int screenHeight) 
			: hDC(NULL), 
			  hRC(NULL),
			  width_(0), 
			  height_(0),
			  size_(0),
			  texture_(0),
			  stretch_(stretch),
			  screenWidth_(screenWidth),
			  screenHeight_(screenHeight), 
			  pboIndex_(0),
			  firstFrame_(true)
		{						
			static PIXELFORMATDESCRIPTOR pfd =				
			{
				sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
				1,											// Version Number
				PFD_SUPPORT_OPENGL |						// Format Must Support OpenGL
				PFD_DOUBLEBUFFER,							// Must Support Double Buffering
				PFD_TYPE_RGBA,								// Request An RGBA Format
				32,											// Select Our Color Depth
				0, 0, 0, 0, 0, 0,							// Color Bits Ignored
				0,											// No Alpha Buffer
				0,											// Shift Bit Ignored
				0,											// No Accumulation Buffer
				0, 0, 0, 0,									// Accumulation Bits Ignored
				0,											// 16Bit Z-Buffer (Depth Buffer)  
				0,											//  No Stencil Buffer
				0,											// No Auxiliary Buffer
				PFD_MAIN_PLANE,								// Main Drawing Layer
				0,											// Reserved
				0, 0, 0										// Layer Masks Ignored
			};

			;
			if(!(hDC = GetDC(hWnd)))
				throw std::exception("Failed To Get Device Context");

			if(!SetPixelFormat(hDC, ChoosePixelFormat(hDC, &pfd),&pfd))
				throw std::exception("Failed To Set Pixel Format");

			if(!(hRC = wglCreateContext(hDC)))
				throw std::exception("Failed To Create Render Context");

			if(!wglMakeCurrent(hDC, hRC))
				throw std::exception("Failed To Activate Render Context");

			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glEnable(GL_TEXTURE_2D);

			dlist_ = glGenLists(1);

			if(glGetError() != GL_NO_ERROR)
				throw std::exception("Failed To Initialize OpenGL");
		}

		~OGLDevice()
		{
			if (hRC)
			{
				wglMakeCurrent(NULL, NULL);
				wglDeleteContext(hRC);
				hRC = NULL;
			}

			if(texture_)
			{
				glDeleteTextures( 1, &texture_);
				texture_ = 0;
			}
			glDeleteBuffers(2, pbos_);
		}

		GLvoid ReSizeGLScene(GLsizei width, GLsizei height)	
		{
			width_ = width;
			height_ = height;
			size_ = width_*height_*4;
			
			glViewport(0, 0, screenWidth_, screenHeight_);

			if(glGetError() != GL_NO_ERROR)
				throw std::exception("Failed To Update Viewport");

			float wratio = (float)width_/(float)width_;
			float hratio = (float)height_/(float)height_;

			std::pair<float, float> targetRatio = None();
			if(stretch_ == ogl::Fill)
				targetRatio = Fill();
			else if(stretch_ == ogl::Uniform)
				targetRatio = Uniform();
			else if(stretch_ == ogl::UniformToFill)
				targetRatio = UniformToFill();

			float wSize = targetRatio.first;
			float hSize = targetRatio.second;

			glNewList(dlist_, GL_COMPILE);
				glBegin(GL_QUADS);
					glTexCoord2f(0.0f,	 hratio);	glVertex2f(-wSize, -hSize);
					glTexCoord2f(wratio, hratio);	glVertex2f( wSize, -hSize);
					glTexCoord2f(wratio, 0.0f);		glVertex2f( wSize,  hSize);
					glTexCoord2f(0.0f,	 0.0f);		glVertex2f(-wSize,  hSize);
				glEnd();	
			glEndList();

			if(texture_ != 0)			
			{
				glDeleteTextures( 1, &texture_);
				texture_ = 0;
			}
			
			glDeleteBuffers(2, pbos_);

			glGenTextures(1, &texture_);	
			glBindTexture( GL_TEXTURE_2D, texture_);

			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width_, height_, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

			if(glGetError() != GL_NO_ERROR)
				throw std::exception("Failed To Create Texture");

			glGenBuffersARB(2, pbos_);
			GLenum error = glGetError();
			glBindBuffer(GL_PIXEL_PACK_BUFFER, pbos_[0]);
			error = glGetError();
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size_, 0, GL_STREAM_DRAW);
			error = glGetError();
			glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbos_[1]);
			glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, size_, 0, GL_STREAM_DRAW);

			error = glGetError();
			if(error != GL_NO_ERROR)
				throw std::exception("Failed To Create PBOs");

			pboIndex_ = 0;
		}

		std::pair<float, float> None()
		{
			float width = (float)width_/(float)screenWidth_;
			float height = (float)height_/(float)screenHeight_;

			return std::make_pair(width, height);
		}

		std::pair<float, float> Uniform()
		{
			float aspect = (float)width_/(float)height_;
			float width = min(1.0f, (float)screenHeight_*aspect/(float)screenWidth_);
			float height = (float)(screenWidth_*width)/(float)(screenHeight_*aspect);

			return std::make_pair(width, height);
		}

		std::pair<float, float> Fill()
		{
			return std::make_pair(1.0f, 1.0f);
		}

		std::pair<float, float> UniformToFill()
		{
			float aspect = (float)width_/(float)height_;

			float wr = (float)width_/(float)screenWidth_;
			float hr = (float)height_/(float)screenHeight_;
			float r_inv = 1.0f/min(wr, hr);

			float width = wr*r_inv;
			float height = hr*r_inv;

			return std::make_pair(width, height);
		}

		void Render(unsigned char* data)
		{					
			// RENDER

			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
			glLoadIdentity();
	
			glBindTexture(GL_TEXTURE_2D, texture_);
			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[pboIndex_]);
	
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width_, height_, GL_BGRA, GL_UNSIGNED_BYTE, 0);

			if(!firstFrame_)
				glCallList(dlist_);		

			// UPDATE

			int nextPboIndex = pboIndex_ ^ 1;

			glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[nextPboIndex]);
			glBufferData(GL_PIXEL_UNPACK_BUFFER, size_, NULL, GL_STREAM_DRAW);
			GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);

			if(ptr != NULL)			
			{
				memcpy(ptr, data, size_);				
				glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			}

			// SWAP

			pboIndex_ = nextPboIndex;
			SwapBuffers(hDC);

			if(firstFrame_)
			{
				firstFrame_ = false;
				Render(data);
			}
		}
	
		int screenWidth_;
		int screenHeight_;

		bool firstFrame_;

		GLuint dlist_;
		GLuint texture_;

		int width_;
		int height_;
		int size_;

		HDC	  hDC;
		HGLRC hRC;
		
		Stretch stretch_;
		GLuint pbos_[2];
		int pboIndex_;
	};

	typedef std::tr1::shared_ptr<OGLDevice> OGLDevicePtr;

	struct OGLPlaybackStrategy: public IFramePlaybackStrategy
	{
		OGLPlaybackStrategy(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl), lastTime_(timeGetTime()), lastFrameCount_(0)
		{}

		FrameManagerPtr GetFrameManager()
		{
			return pConsumerImpl_->pFrameManager_;
		}	
		FramePtr GetReservedFrame()
		{
			return pConsumerImpl_->pFrameManager_->CreateFrame();
		}

		void DisplayFrame(Frame* pFrame)
		{
			DWORD timediff = timeGetTime() - lastTime_;
			if(timediff < 35)
				Sleep(40 - timediff);
			lastTime_ = timeGetTime();

			// Check if frame is valid and if it has already been rendered
			if(pFrame == NULL || (pFrame->ID() == lastFrameID_ && lastFrameCount_ > 1)) // Potential problem is that if the HDC is invalidated by external application it will stay that way, (R.N), keep or remove?
				return;		

			lastFrameCount_ = pFrame->ID() == lastFrameID_ ? ++lastFrameCount_ : 0; // Cant stop rendering until 2 frames are pushed due to doublebuffering

			if(!pOGLDevice_)
			{
				pOGLDevice_.reset(new OGLDevice(pConsumerImpl_->hWnd_, pConsumerImpl_->stretch_, pConsumerImpl_->screenWidth_, pConsumerImpl_->screenHeight_));
				pOGLDevice_->ReSizeGLScene(pConsumerImpl_->fmtDesc_.width, pConsumerImpl_->fmtDesc_.height);
			}

			pOGLDevice_->Render(pFrame->GetDataPtr());

			lastFrameID_ = pFrame->ID();
		}

		int lastFrameCount_;
		utils::ID lastFrameID_;
		OGLDevicePtr pOGLDevice_;
		Implementation* pConsumerImpl_;
		DWORD lastTime_;	
	};
	
	Implementation(HWND hWnd, const FrameFormatDescription& fmtDesc, unsigned int screenIndex, Stretch stretch) 
		: hWnd_(hWnd), fmtDesc_(fmtDesc), pFrameManager_(new SystemFrameManager(fmtDesc_)), stretch_(stretch), screenIndex_(screenIndex)
	{
		bool succeeded = SetupDevice();
		assert(succeeded);
	}

	~Implementation()
	{
		bool succeeded = ReleaseDevice();
		assert(succeeded);
	}

	bool SetupDevice()
	{
		DISPLAY_DEVICE dDevice;			
		memset(&dDevice,0,sizeof(dDevice));
		dDevice.cb = sizeof(dDevice);

		std::vector<DISPLAY_DEVICE> displayDevices;
		for(int n = 0; EnumDisplayDevices(NULL, n, &dDevice, NULL); ++n)
		{
			displayDevices.push_back(dDevice);
			memset(&dDevice,0,sizeof(dDevice));
			dDevice.cb = sizeof(dDevice);
		}

		if(screenIndex_ >= displayDevices.size())
			return false;
		
		if(!GetWindowRect(hWnd_, &prevRect_))
			throw std::exception("Failed to get Window Rectangle.");

		DEVMODE devmode;
		memset(&devmode,0,sizeof(devmode));
		
		if(!EnumDisplaySettings(displayDevices[screenIndex_].DeviceName, ENUM_CURRENT_SETTINGS, &devmode))
		{
			std::stringstream msg;
			msg << "Failed to enumerate Display Settings for DisplayDevice " << screenIndex_ << ".";
			throw std::exception(msg.str().c_str());
		}

		prevMode_ = devmode;

		screenWidth_ = devmode.dmPelsWidth;
		screenHeight_ = devmode.dmPelsHeight;

		ChangeDisplaySettings(&devmode, CDS_FULLSCREEN);

		//if(result != DISP_CHANGE_SUCCESSFUL)
		//	throw std::exception("Failed to change Display Settings.");

		prevStyle_   = GetWindowLong(hWnd_, GWL_STYLE);
		prevExStyle_ = GetWindowLong(hWnd_, GWL_EXSTYLE);
	
		if(!(SetWindowLong(hWnd_, GWL_STYLE, WS_POPUP) && SetWindowLong(hWnd_, GWL_EXSTYLE, WS_EX_APPWINDOW | WS_EX_TOPMOST)))
			throw std::exception("Failed to change window style.");

		if(!MoveWindow(hWnd_, devmode.dmPosition.x, devmode.dmPosition.y, screenWidth_, screenHeight_, TRUE))
			throw std::exception("Failed to move window to display device.");

		ShowWindow(hWnd_,SW_SHOW);						// Show The Window
		SetForegroundWindow(hWnd_);						// Slightly Higher Priority
		

		pPlaybackControl_.reset(new FramePlaybackControl(FramePlaybackStrategyPtr(new OGLPlaybackStrategy(this))));
		pPlaybackControl_->Start();

		LOG << TEXT("OGL INFO: Successfully initialized device ");
		return true;
	}

	bool ReleaseDevice()
	{
		pPlaybackControl_->Stop();
		pPlaybackControl_.reset();

		SetWindowLong(hWnd_, GWL_STYLE, prevStyle_);
		SetWindowLong(hWnd_, GWL_EXSTYLE, prevExStyle_);

		ChangeDisplaySettings(&prevMode_, 0);

		MoveWindow(hWnd_, prevRect_.bottom, prevRect_.left, (prevRect_.right - prevRect_.left), (prevRect_.top-prevRect_.bottom), TRUE);
		
		LOG << TEXT("OGL INFO: Successfully released device ") << utils::LogStream::Flush;
		return true;
	}

	unsigned int screenIndex_;
	int screenWidth_;
	int screenHeight_;
	
	DEVMODE prevMode_;
	RECT  prevRect_;
	DWORD prevExStyle_;
	DWORD prevStyle_;

	Stretch stretch_;
	FrameFormatDescription fmtDesc_;
	HWND hWnd_;
	SystemFrameManagerPtr pFrameManager_;
	FramePlaybackControlPtr pPlaybackControl_;	
};

OGLVideoConsumer::OGLVideoConsumer(HWND hWnd, const FrameFormatDescription& fmtDesc, unsigned int screenIndex, Stretch stretch)
: pImpl_(new Implementation(hWnd, fmtDesc, screenIndex, stretch))
{
}

OGLVideoConsumer::~OGLVideoConsumer(void)
{
}

IPlaybackControl* OGLVideoConsumer::GetPlaybackControl() const
{
	return pImpl_->pPlaybackControl_.get();
}

void OGLVideoConsumer::EnableVideoOutput(){}

void OGLVideoConsumer::DisableVideoOutput(){}

bool OGLVideoConsumer::SetupDevice(unsigned int deviceIndex)
{
	return pImpl_->SetupDevice();
}

bool OGLVideoConsumer::ReleaseDevice()
{
	return pImpl_->ReleaseDevice();
}

const TCHAR* OGLVideoConsumer::GetFormatDescription() const
{
	return pImpl_->fmtDesc_.name;
}

}
}