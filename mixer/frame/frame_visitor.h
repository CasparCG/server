#pragma once

namespace caspar{ namespace core{

class draw_frame;
class write_frame;

struct frame_visitor
{
	virtual void begin(const draw_frame& frame) = 0;
	virtual void visit(write_frame& frame) = 0;
	virtual void end() = 0;
};

}}