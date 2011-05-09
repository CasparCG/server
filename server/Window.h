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