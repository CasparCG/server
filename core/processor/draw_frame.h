#pragma once

#include "fwd.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/operators.hpp>

#include <memory>
#include <vector>
#include <type_traits>

#include "../../common/utility/safe_ptr.h"

namespace caspar { namespace core {
		
class draw_frame : boost::noncopyable
{	
public:
	virtual ~draw_frame(){}

	virtual const std::vector<short>& audio_data() const = 0;		
	virtual void begin_write() = 0;
	virtual void end_write() = 0;
	virtual void draw(frame_shader& shader) = 0;

	static safe_ptr<draw_frame> eof()
	{
		struct eof_frame : public draw_frame
		{
			virtual const std::vector<short>& audio_data() const
			{
				static std::vector<short> audio_data;
				return audio_data;
			}
			virtual void begin_write(){}
			virtual void end_write(){}
			virtual void draw(frame_shader&){}
		};
		static safe_ptr<draw_frame> frame = make_safe<eof_frame>();
		return frame;
	}

	static safe_ptr<draw_frame> empty()
	{
		struct empty_frame : public draw_frame
		{
			virtual const std::vector<short>& audio_data() const
			{
				static std::vector<short> audio_data;
				return audio_data;
			}
			virtual void begin_write(){}
			virtual void end_write(){}
			virtual void draw(frame_shader&){}
		};
		static safe_ptr<draw_frame> frame = make_safe<empty_frame>();
		return frame;
	}
};


}}