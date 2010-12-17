#pragma once

#include "fwd.h"

#include <boost/noncopyable.hpp>

#include "../../common/utility/safe_ptr.h"

namespace caspar { namespace core {
		
class draw_frame : boost::noncopyable
{	
public:
	virtual ~draw_frame(){}
	
	static safe_ptr<draw_frame> eof()
	{
		struct eof_frame : public draw_frame
		{
			virtual void process_image(image_processor&){}
			virtual void process_audio(audio_processor&){}
		};
		static safe_ptr<draw_frame> frame = make_safe<eof_frame>();
		return frame;
	}

	static safe_ptr<draw_frame> empty()
	{
		struct empty_frame : public draw_frame
		{
			virtual void process_image(image_processor&){}
			virtual void process_audio(audio_processor&){}
		};
		static safe_ptr<draw_frame> frame = make_safe<empty_frame>();
		return frame;
	}

	virtual void process_image(image_processor& processor) = 0;
	virtual void process_audio(audio_processor& processor) = 0;
};



}}