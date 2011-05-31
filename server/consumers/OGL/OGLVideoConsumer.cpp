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
		OGLDevice(HWND hWnd, bool fullscreen, bool aspect, int screenWidth, int screenHeight) : hDC(NULL), hRC(NULL), width_(0), height_(0), texture_(0), temp_(NULL), fullscreen_(fullscreen), aspect_(aspect), screenWidth_(screenWidth), screenHeight_(screenHeight)//, pboIndex_(0)
		{						
			static PIXELFORMATDESCRIPTOR pfd =				// pfd Tells Windows How We Want Things To Be
			{
				sizeof(PIXELFORMATDESCRIPTOR),				// Size Of This Pixel Format Descriptor
				1,											// Version Number
				PFD_DRAW_TO_WINDOW |						// Format Must Support Window
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
			if(temp_ != NULL)			
			{
				delete[] temp_;	
				temp_ = NULL;
			}
			//glDeleteBuffers(2, pbos_);
		}

		// clamp to nearest power of 2
		int clp2(int x)
		{
			x = x - 1;
			x = x | (x >> 1);
			x = x | (x >> 2);
			x = x | (x >> 4);
			x = x | (x >> 8);
			x = x | (x >> 16);
			return x+1;
		}

		GLvoid ReSizeGLScene(GLsizei width, GLsizei height)	
		{
			width_ = width;
			height_ = height;
			
			glViewport(0, 0, screenWidth_, screenHeight_);

			if(glGetError() != GL_NO_ERROR)
				throw std::exception("Failed To Update Viewport");

			width2_ = clp2(width);
			height2_ = clp2(height);
			
			float wratio = (float)width_/(float)width2_;
			float hratio = (float)height_/(float)height2_;

			float hSize = 1.0f;
			float wSize = 1.0f;

			if(aspect_)
			{
				std::pair<float, float> p = FitScreenAspect((float)width/(float)height);		

				wSize = 1.0f-((float)screenWidth_-p.first)/(float)screenWidth_;
				hSize = 1.0f-((float)screenHeight_-p.second)/(float)screenHeight_;
			}

			glNewList(dlist_, GL_COMPILE);
				glBegin(GL_QUADS);
					glTexCoord2f(0.0f,	 hratio);	glVertex2f(-wSize, -hSize);
					glTexCoord2f(wratio, hratio);	glVertex2f( wSize, -hSize);
					glTexCoord2f(wratio, 0.0f);		glVertex2f( wSize,  hSize);
					glTexCoord2f(0.0f,	 0.0f);		glVertex2f(-wSize,  hSize);
				glEnd();	
			glEndList();

			if(temp_ != NULL)			
			{
				delete[] temp_;	
				temp_ = NULL;
			}

			temp_ = new unsigned char[width2_*height2_*4];
			memset(temp_, 0, width2_*height2_*4);

			if(texture_ != 0)			
			{
				glDeleteTextures( 1, &texture_);
				texture_ = 0;
			}

			glGenTextures(1, &texture_);	
			glBindTexture( GL_TEXTURE_2D, texture_);

			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
			glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

			glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8, width2_, height2_, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);

			if(glGetError() != GL_NO_ERROR)
				throw std::exception("Failed To Create Texture");

			// TODO: Add PBO support
			//GLuint pbo;
			//glGenBuffersARB(2, &pbosIds_);
			//error = glGetError();
			//glBindBuffer(GL_PIXEL_PACK_BUFFER, pbosIds_[0]);
			//error = glGetError();
			//glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width2_*height2_*4, 0, GL_STREAM_DRAW);
			//error = glGetError();
			//glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_ARB, pbosIds_[1]);
			//glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_ARB, width2_*height2_*4, 0, GL_STREAM_DRAW);

			//error = glGetError();
			//if(error != GL_NO_ERROR)
			//	throw std::exception("Failed To Create PBOs");

			//pboIndex_ = 0;
		}

		std::pair<float, float> FitScreenAspect(float frameAspect)
		{
			float height = (float)screenWidth_/frameAspect;
			float width = (float)screenHeight_;

			height = min(height, screenHeight_);
			width = height*frameAspect;
			width = min(width, screenWidth_);
			height = width/frameAspect;

			return std::make_pair(width, height);
		}

		void Render(unsigned char* data)
		{					
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			glLoadIdentity();
	
			glBindTexture(GL_TEXTURE_2D, texture_);
			//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbos_[pboIndex_]);

			for(int row = 0; row < height_; ++row)				
				memcpy(temp_+row*width2_*4, data+row*width_*4, width_*4);			
	
			glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width2_, height2_, GL_BGRA, GL_UNSIGNED_BYTE, temp_);

			glCallList(dlist_);
			
			//int nextPboIndex = (pboIndex_ + 1) & ~1;

			//glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbosIds_[nextPboIndex]);
			//glBufferData(GL_PIXEL_UNPACK_BUFFER, width2_*height2_*4, 0, GL_STREAM_DRAW);
			//GLubyte* ptr = (GLubyte*)glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
			//if(ptr)
			//{
			//	for(int row = 0; row < height_; ++row)				
			//		memcpy(ptr+row*width2_, data+row*width_, width_);			
			//	glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
			//}

			//pboIndex_ = nextPboIndex;

			SwapBuffers(hDC);
		}
	
		int screenWidth_;
		int screenHeight_;

		GLuint dlist_;
		GLuint texture_;

		int width2_;
		int height2_;

		int width_;
		int height_;
		HDC	  hDC;
		HGLRC hRC;

		unsigned char* temp_;
		
		bool fullscreen_;
		bool aspect_;
		//GLuint pbos_[2];
		//int pboIndex_;
	};

	typedef std::tr1::shared_ptr<OGLDevice> OGLDevicePtr;

	struct OGLPlaybackStrategy: public IFramePlaybackStrategy
	{
		OGLPlaybackStrategy(Implementation* pConsumerImpl) : pConsumerImpl_(pConsumerImpl), lastTime_(timeGetTime())
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
			if(pFrame == NULL || pFrame->ID() == lastFrameID_)
				return;		

			if(!pOGLDevice_)
			{
				pOGLDevice_.reset(new OGLDevice(pConsumerImpl_->hWnd_, pConsumerImpl_->fullscreen_, pConsumerImpl_->aspect, pConsumerImpl_->screenWidth_, pConsumerImpl_->screenHeight_));
				pOGLDevice_->ReSizeGLScene(pConsumerImpl_->fmtDesc_.width, pConsumerImpl_->fmtDesc_.height);
			}

			pOGLDevice_->Render(pFrame->GetDataPtr());

			lastFrameID_ = pFrame->ID();
		}

		utils::ID lastFrameID_;
		OGLDevicePtr pOGLDevice_;
		Implementation* pConsumerImpl_;
		DWORD lastTime_;	
	};
	
	Implementation(HWND hWnd, const FrameFormatDescription& fmtDesc, unsigned int screenIndex, bool fullscreen, bool aspect) : hWnd_(hWnd), fmtDesc_(fmtDesc), pFrameManager_(new BitmapFrameManager(fmtDesc_, hWnd)), fullscreen_(fullscreen), aspect(aspect), screenIndex_(screenIndex)
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
		if(fullscreen_)
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
				throw std::exception("Failed to get Display Settings for DisplayDevice.");

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
		}

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

	bool aspect;
	FrameFormatDescription fmtDesc_;
	HWND hWnd_;
	BitmapFrameManagerPtr pFrameManager_;
	FramePlaybackControlPtr pPlaybackControl_;
	bool fullscreen_;
	
};

OGLVideoConsumer::OGLVideoConsumer(HWND hWnd, const FrameFormatDescription& fmtDesc, unsigned int screenIndex, bool fullscreen, bool aspect) : pImpl_(new Implementation(hWnd, fmtDesc, screenIndex, fullscreen, aspect))
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