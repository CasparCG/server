#include "stdafx.h"

#include "window.h"
#include "resource.h"

namespace caspar {

using namespace utils;

Window::Window() : _hwnd(NULL), _hinstance(NULL), _hdc(NULL)
{
}

Window::~Window()
{
	Destroy();
}

bool Window::Initialize(HINSTANCE hinstance, const TCHAR* windowTitle, const TCHAR* className)
{
	_hinstance = hinstance;
	_classname = className;

    WNDCLASSEX wndClass;         // Window class
    ZeroMemory(&wndClass, sizeof(wndClass)); // Clear the window class structure
    wndClass.cbSize = sizeof(WNDCLASSEX); 
    wndClass.style          = CS_HREDRAW | CS_VREDRAW | CS_CLASSDC;
    wndClass.lpfnWndProc    = (WNDPROC)WndProc;
    wndClass.cbClsExtra     = 0;
    wndClass.cbWndExtra     = 0;
    wndClass.hInstance      = _hinstance;
    wndClass.hIcon          = LoadIcon(_hinstance, MAKEINTRESOURCE(IDI_ICON1));
    wndClass.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wndClass.hbrBackground  = (HBRUSH)(COLOR_WINDOW+1);
    wndClass.lpszMenuName   = NULL;//MAKEINTRESOURCE(IDR_MAINMENU);
    wndClass.lpszClassName  = _classname.c_str();
    wndClass.hIconSm        = 0;

    if (RegisterClassEx(&wndClass) == 0)// Attemp to register the window class
    {
		LOG << TEXT("WINDOW ERROR: Failed to register the window class!") << LogStream::Flush;
        return false;
    }
    DWORD dwStyle;              // Window styles
    DWORD dwExStyle;            // Extended window styles

    dwStyle = WS_OVERLAPPEDWINDOW |        // Creates an overlapping window
              WS_CLIPCHILDREN |            // Doesn"t draw within child windows
              WS_CLIPSIBLINGS;              // Doesn"t draw within sibling windows
    dwExStyle = WS_EX_APPWINDOW |          // Top level window
                WS_EX_WINDOWEDGE;           // Border with a raised edge
    
    //adjust window size
    RECT rMain;
    rMain.left = 0;
    rMain.right = 720;
    rMain.top = 0;
    rMain.bottom = 576;  

    AdjustWindowRect(&rMain, dwStyle, 0);

    // Attempt to create the actual window
    _hwnd = CreateWindowEx( dwExStyle,       // Extended window styles
                            _classname.c_str(),   // Class name
                            windowTitle,       // Window title (caption)
                            dwStyle,         // Window styles
                            0, 0,            // Window position
                            rMain.right - rMain.left,
                            rMain.bottom - rMain.top,   // Size of window
                            0,               // No parent window
                            0,               // No menu
                            _hinstance,      // Instance
                            0);            // Pass nothing to WM_CREATE

    if(_hwnd == 0) 
    {
        Destroy();
        LOG << TEXT("WINDOW ERROR: Unable to create window!") << LogStream::Flush;
        return false;
    }

    ShowWindow(_hwnd, SW_SHOW);
    SetForegroundWindow(_hwnd);
    SetFocus(_hwnd);


	//TEST: select a more appropriate pixelformat
	_hdc = ::GetDC(_hwnd);

	PIXELFORMATDESCRIPTOR pfd = 
	{
		sizeof(PIXELFORMATDESCRIPTOR),		//size of struct
		1,									//version number
		//PFD_DRAW_TO_WINDOW |				//Format must support draw to window
		PFD_DRAW_TO_BITMAP |				//Format must support draw to bitmap
		PFD_DOUBLEBUFFER_DONTCARE |			//Format does not have to support doublebuffer
		PFD_DEPTH_DONTCARE,					//Formet does not have to support depthbuffer
		PFD_TYPE_RGBA,						//Request RGBA format
		24,									//Color depth
		0,0,0,0,0,0,						//colorbits ignored
		8,									//8-bit alpha-buffer
		0,									//shift bit ignored
		0,									//no accumulation-buffer
		0,0,0,0,							//Accumulation bits ignored
		0,									//no depth-buffer
		0,									//no stencil-buffer
		0,									//no auxiliary-buffer
		PFD_MAIN_PLANE,						//Main drawing layer
		0,									//RESERVED
		0,0,0								//Layer masks ignored
	};

	unsigned int nPixelFormat = ChoosePixelFormat(_hdc, &pfd);
	if(nPixelFormat) {
		if(!SetPixelFormat(_hdc, nPixelFormat, &pfd)) {
			;
		}
	}
	//END TEST: select a more appropriate pixelformat
/*
	//TEST: give flash access to a directdraw device
	IDirectDrawFactory* pDDF = NULL;
	pDD_ = NULL;
	CComBSTR ddfGUID(_T("{4FD2A832-86C8-11d0-8FCA-00C04FD9189D}"));
	CLSID clsid;
	HRESULT hr = CLSIDFromString((LPOLESTR)ddfGUID, &clsid);

	hr = CoCreateInstance(clsid, NULL, CLSCTX_INPROC_SERVER, GUID_DDFactory, (void **)&pDDF);
	if(pDDF != 0) {
		pDDF->CreateDirectDraw(NULL, _hwnd, DDSCL_NORMAL, NULL, NULL, &pDD_);
		pDDF->Release();
	}
	//END TEST: give flash access to a directdraw device
*/
	return true;
}

void Window::Destroy()
{
/* 	if(pDD_ != 0) {
 		pDD_->Release();
		pDD_ = 0;
	}
*/
	// Attempts to destroy the window
	if(_hwnd) {
		DestroyWindow(_hwnd);
		_hwnd = NULL;
	}

    // Attempts to unregister the window class
    if (!UnregisterClass(_classname.c_str(), _hinstance))
    {
        LOG << TEXT("WINDOW ERROR: Unable to unregister window class!") << LogStream::Flush;
        _hinstance = NULL;
    }
}

}