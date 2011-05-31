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
