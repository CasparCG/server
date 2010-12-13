#include "../stdafx.h"

#include "draw_frame.h"

#include "frame_shader.h"

namespace caspar { namespace core {
	
struct null_frame : public detail::draw_frame_impl
{
	virtual const std::vector<short>& audio_data() const
	{
		static std::vector<short> no_audio;
		return no_audio;
	}
	virtual void begin_write(){}
	virtual void end_write(){}
	virtual void draw(frame_shader&){}
};

static const std::shared_ptr<null_frame>& get_empty()
{
	static auto empty = std::make_shared<null_frame>();
	return empty;
}
	
static const std::shared_ptr<null_frame>& get_eof()
{
	static auto eof = std::make_shared<null_frame>();
	return eof;
}

draw_frame::draw_frame() : impl_(get_empty()){}
draw_frame::draw_frame(const draw_frame& other) : impl_(other.impl_){}
draw_frame::draw_frame(draw_frame&& other) : impl_(std::move(other.impl_)){other.impl_ = get_empty();}
draw_frame::draw_frame(eof_frame){impl_ = get_eof();}
draw_frame::draw_frame(empty_frame){impl_ = get_empty();}

void draw_frame::swap(draw_frame& other){impl_.swap(other.impl_);}
		
bool draw_frame::operator==(const eof_frame&){return impl_ == get_eof();}
bool draw_frame::operator==(const empty_frame&){return impl_ == get_empty();}
bool draw_frame::operator==(const draw_frame& other){return impl_ == other.impl_;}
	
draw_frame& draw_frame::operator=(const draw_frame& other)
{
	draw_frame temp(other);
	temp.swap(*this);
	return *this;
}

draw_frame& draw_frame::operator=(draw_frame&& other)
{
	draw_frame temp(other);
	temp.swap(*this);
	return *this;
}

draw_frame& draw_frame::operator=(eof_frame&&)
{
	impl_ = get_eof();
	return *this;
}	

draw_frame& draw_frame::operator=(empty_frame&&)
{
	impl_ = get_empty();
	return *this;
}	

eof_frame draw_frame::eof(){return eof_frame();}
empty_frame draw_frame::empty(){return empty_frame();}

const std::vector<short>& draw_frame::audio_data() const{return impl_->audio_data();}	

void draw_frame::begin_write(){impl_->begin_write();}
void draw_frame::end_write(){impl_->end_write();}
void draw_frame::draw(frame_shader& shader){impl_->draw(shader);}

}}