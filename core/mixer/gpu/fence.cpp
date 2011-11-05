#include "../../StdAfx.h"

#include "fence.h"

#include "ogl_device.h"

#include <common/gl/gl_check.h>

#include <gl/glew.h>

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
			while(!ogl.invoke([this]{return ready();}, normal_priority) && delay < 20)
			{
				delay += 2;
				Sleep(2);
			}
		}

		static tbb::atomic<bool> warned;
		
		if(delay > 2)
		{
			if(!warned.fetch_and_store(true))
			{
				CASPAR_LOG(warning) << L"[fence] Performance warning. GPU was not ready during requested host read-back. Delayed by atleast: " << delay << L" ms. Further warnings are sent to trace log level."
									<< L" You can ignore this warning if you do not notice any problems with output video. This warning is caused by insufficent support or performance of your graphics card for OpenGL based memory transfers. "
									<< L" Please try to update your graphics drivers or update your graphics card, see recommendations on (www.casparcg.com)."
									<< L" Further help is available at (www.casparcg.com/forum).";
			}
			else
				CASPAR_LOG(trace) << L"[fence] Performance warning. GPU was not ready during requested host read-back. Delayed by atleast: " << delay << L" ms.";
		}
	}
};
	
fence::fence() 
{
	static bool log_flag = false;

	if(GLEW_ARB_sync)
		impl_.reset(new implementation());
	else if(!log_flag)
	{
		CASPAR_LOG(warning) << "[fence] GL_SYNC not supported, running without fences. This might cause performance degradation when running multiple channels and short buffer depth.";
		log_flag = true;
	}
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