#pragma once

#include <core/producer/frame/frame_visitor.h>

#include <boost/noncopyable.hpp>

namespace caspar { namespace core {

class audio_transform;

class audio_mixer : public core::frame_visitor, boost::noncopyable
{
public:
	audio_mixer();

	virtual void begin(const basic_frame& frame);
	virtual void visit(write_frame& frame);
	virtual void end();

	std::vector<short> begin_pass();
	void end_pass();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}