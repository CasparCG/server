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
		
struct draw_frame_decorator;

class draw_frame : boost::noncopyable
{	
	friend struct draw_frame_decorator;
	friend class frame_renderer;
public:
	virtual ~draw_frame(){}

	virtual const std::vector<short>& audio_data() const = 0;		

	static safe_ptr<draw_frame> eof()
	{
		struct eof_frame : public draw_frame
		{
			virtual const std::vector<short>& audio_data() const
			{
				static std::vector<short> audio_data;
				return audio_data;
			}
			virtual void prepare(){}
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
			virtual void prepare(){}
			virtual void draw(frame_shader&){}
		};
		static safe_ptr<draw_frame> frame = make_safe<empty_frame>();
		return frame;
	}
private:
	virtual void prepare() = 0;
	virtual void draw(frame_shader& shader) = 0;
};

struct draw_frame_decorator
{
protected:
	static void prepare(const safe_ptr<draw_frame>& frame) {frame->prepare();}
	static void draw(const safe_ptr<draw_frame>& frame, frame_shader& shader) {frame->draw(shader);}
};


}}