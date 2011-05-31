#ifndef _CASPAR_MEDIACONTROLLER_H__
#define _CASPAR_MEDIACONTROLLER_H__

namespace caspar {

class IMediaController
{
	IMediaController(const IMediaController&);
	IMediaController& operator=(const IMediaController&);

protected:
	IMediaController() {}

public:
	virtual ~IMediaController() {}

};

}	//namespace caspar

#endif	//_CASPAR_MEDIACONTROLLER_H__