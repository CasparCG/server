#pragma once

#include <memory>

namespace caspar { namespace core {
	
class ogl_device;

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
