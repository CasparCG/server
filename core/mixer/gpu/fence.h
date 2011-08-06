#pragma once

namespace caspar { namespace core {
	
class ogl_device;

class fence
{
	unsigned int id_;
public:
	fence();
	~fence();
	void set();
	bool ready() const;
	void wait(ogl_device& ogl);
};

}}
