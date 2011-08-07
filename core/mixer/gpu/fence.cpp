#include "../../StdAfx.h"

#include "fence.h"

#include "ogl_device.h"

#include <common/gl/gl_check.h>

namespace caspar { namespace core {

struct fence::implementation
{
	GLsync sync_;

	implementation() : sync_(0){}
	~implementation(){glDeleteSync(sync_);}
		
	void set()
	{
		glDeleteSync(sync_);
		sync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
	}

	bool ready() const
	{
		if(!sync_)
			return true;

		GLsizei length = 0;
		int values[] = {0};

		GL(glGetSynciv(sync_, GL_SYNC_STATUS, 1, &length, values));

		return values[0] == GL_SIGNALED;
	}

	void wait(ogl_device& ogl)
	{	
		int delay = 0;
		if(!ogl.invoke([this]{return ready();}, high_priority))
		{
			while(!ogl.invoke([this]{return ready();}, normal_priority) && delay < 30)
			{
				delay += 3;
				Sleep(3);
			}
		}

		if(delay > 0)
			CASPAR_LOG(warning) << L"[fence] Performance warning. GPU was not ready during requested host read-back. Delayed by atleast: " << delay << L" ms.";
	}
};
	
fence::fence() 
{
	if(GLEW_ARB_sync)
		impl_.reset(new implementation());
	else
		CASPAR_LOG(warning) << "[fence] GL_SYNC not supported, running without fences. This might cause performance degradation when running multiple channels and short buffer depth.";

}

void fence::set() 
{
	if(impl_)
		impl_->set();
}

bool fence::ready() const 
{
	return impl_ ? impl_->ready() : true;
}

void fence::wait(ogl_device& ogl)
{
	if(impl_)
		impl_->wait(ogl);
}

}}