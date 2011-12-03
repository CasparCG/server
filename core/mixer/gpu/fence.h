/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
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

#pragma once

#include <memory>

namespace caspar { namespace core {
	
class ogl_device;

// Used to avoid blocking ogl thread for async operations. 
// This is imported when several objects use the same ogl context.
// Based on http://www.opengl.org/registry/specs/ARB/sync.txt.
class fence
{
public:
	fence();
	void set();
	bool ready() const;
	void wait(ogl_device& ogl);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}
