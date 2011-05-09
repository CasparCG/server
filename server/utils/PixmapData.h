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
 
#ifndef __IMAGEDATA_H__
#define __IMAGEDATA_H__

#include <memory>

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

namespace caspar {
namespace utils {

class PixmapData
{
	PixmapData(const PixmapData&);
	PixmapData& operator=(const PixmapData&);

public:
	PixmapData() : height(0), width(0), bpp(0), pData_(0) 
	{}
	PixmapData(int w, int h, int b) : width(w), height(h), bpp(b), pData_(new unsigned char[w*h*b])
	{}

	~PixmapData() {
		Clear();
	}

	void Set(int w, int h, int b) {
		Clear();
		width = w;
		height = h;
		bpp = b;
		pData_ = new unsigned char[w*h*b];
	}

	void Clear() {
		height = 0;
		width = 0;
		bpp = 0;

		if(pData_ != 0)
		{
			delete[] pData_;
			pData_ = 0;
		}
	}

	unsigned char* GetDataPtr() {
		return pData_;
	}

	int height;
	int width;
	int bpp;

private:
	unsigned char *pData_;
};

typedef std::tr1::shared_ptr<PixmapData> PixmapDataPtr;

}	//namespace utils
}	//namespace caspar

#endif //__IMAGEDATA_H__
