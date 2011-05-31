#ifndef _DCWRAPPER_H_
#define _DCWRAPPER_H_

namespace caspar
{

class DCWrapper
{
public:
	explicit DCWrapper(HWND hWnd);
	~DCWrapper();
	
	operator HDC() {
		return dc_;
	}

private:
	HWND hWnd_;
	HDC dc_;
};

}

#endif