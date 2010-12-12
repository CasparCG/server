#include "../stdafx.h"

#include "draw_frame.h"

#include "frame_shader.h"

namespace caspar { namespace core {
	
draw_frame::draw_frame() : tag_(empty_tag){}
draw_frame::draw_frame(const draw_frame& other) : impl_(other.impl_), tag_(other.tag_){}
draw_frame::draw_frame(draw_frame&& other) : impl_(std::move(other.impl_)), tag_(other.tag_)
{
	other.tag_ = empty_tag;
}

const std::vector<short>& draw_frame::audio_data() const 
{
	static std::vector<short> no_audio;
	return impl_ ? impl_->audio_data() : no_audio;
}	

void draw_frame::swap(draw_frame& other)
{
	impl_.swap(other.impl_);
	std::swap(tag_, other.tag_);
}
	
void draw_frame::begin_write()
{
	if(impl_)
		impl_->begin_write();
}

void draw_frame::end_write()
{
	if(impl_)
		impl_->end_write();
}

void draw_frame::draw(frame_shader& shader)
{
	if(impl_)
		impl_->draw(shader);
}

eof_frame draw_frame::eof(){return eof_frame();}
empty_frame draw_frame::empty(){return empty_frame();}
	
bool draw_frame::operator==(const eof_frame&){return tag_ == eof_tag;}
bool draw_frame::operator==(const empty_frame&){return tag_ == empty_tag;}
bool draw_frame::operator==(const draw_frame& other){return impl_ == other.impl_ && tag_ == other.tag_;}
	
draw_frame& draw_frame::operator=(const draw_frame& other)
{
	draw_frame temp(other);
	temp.swap(*this);
	return *this;
}
draw_frame& draw_frame::operator=(draw_frame&& other)
{
	impl_ = std::move(other.impl_);
	tag_ = other.tag_;
	other.tag_ = empty_tag;
	return *this;
}
draw_frame& draw_frame::operator=(eof_frame&&)
{
	impl_ = nullptr;
	tag_ = eof_tag;
	return *this;
}	
draw_frame& draw_frame::operator=(empty_frame&&)
{
	impl_ = nullptr;
	tag_ = empty_tag;
	return *this;
}	

}}