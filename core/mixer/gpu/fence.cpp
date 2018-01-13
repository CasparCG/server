/*
* Copyright 2013 Sveriges Television AB http://casparcg.com/
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../StdAfx.h"

#include "fence.h"

#include "ogl_device.h"

#include <common/gl/gl_check.h>

#include <gl/glew.h>

namespace caspar { namespace core {

struct fence::implementation
{		
	void set()
	{
	}

	bool ready() const
	{
		return true;
	}

	void wait(ogl_device& ogl)
	{	
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