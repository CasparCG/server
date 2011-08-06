#include "../../StdAfx.h"

#include "fence.h"

#include "ogl_device.h"

#include <common/gl/gl_check.h>

namespace caspar { namespace core {
	
fence::fence() : id_(0){}

fence::~fence()
{
	if(id_)
		glDeleteFencesNV(1, &id_);
}

void fence::set()
{
	if(id_)
		glDeleteFencesNV(1, &id_);
			
	GL(glGenFencesNV(1, &id_));
	GL(glSetFenceNV(id_, GL_ALL_COMPLETED_NV));
}

bool fence::ready() const
{
	return id_ ? GL2(glTestFenceNV(id_)) != GL_FALSE : true;
}

void fence::wait(ogl_device& ogl)
{	
	int delay = 0;
	if(!ogl.invoke([this]{return ready();}, high_priority))
	{
		while(!ogl.invoke([this]{return ready();}, normal_priority))
		{
			delay += 3;
			Sleep(3);
		}
	}

	if(delay > 0)
		CASPAR_LOG(warning) << L"[ogl_device] Performance warning. GPU was not ready during requested host read-back. Delayed by atleast: " << delay << L" ms.";
}

}}