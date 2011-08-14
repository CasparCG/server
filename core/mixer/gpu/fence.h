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
