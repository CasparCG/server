#pragma once

namespace caspar{ namespace core{

class basic_frame;
class write_frame;

struct frame_visitor
{
	virtual void begin(const basic_frame& frame) = 0;
	virtual void visit(write_frame& frame) = 0;
	virtual void end() = 0;
};

}}