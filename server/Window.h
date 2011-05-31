
#pragma once

#include <string>
#include <memory>
//struct IDirectDraw;

//extern const GUID GUID_DDFactory;
//extern const GUID GUID_DD3;

namespace caspar
{
	class Window
	{
	public:
		Window();
		virtual ~Window();

		bool Initialize(HINSTANCE, const TCHAR* windowTitle, const TCHAR* className);
		void Destroy();

		HWND getHwnd()
		{
			return _hwnd;
		}
		HINSTANCE getInstance()
		{
			return _hinstance;
		}

//		IDirectDraw* pDD_;
	private:
		HDC			_hdc;
		HWND		_hwnd;
		HINSTANCE	_hinstance;
		tstring	_classname;
	};

	typedef std::tr1::shared_ptr<Window> WindowPtr;

}	//namespace caspar