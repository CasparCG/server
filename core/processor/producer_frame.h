#pragma once

#include "write_frame.h"
#include "composite_frame.h"
#include "transform_frame.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/variant.hpp>
#include <boost/utility/enable_if.hpp>

#include <memory>
#include <vector>
#include <type_traits>

namespace caspar { namespace core {
		
struct eof_frame{};
struct empty_frame{};

class producer_frame
{
	enum frame_type
	{
		normal_type,
		eof_type,
		empty_type
	};
public:
	producer_frame() : type_(empty_type){}
	producer_frame(const producer_frame& other) : frame_(other.frame_), type_(other.type_){}
	producer_frame(producer_frame&& other) : frame_(std::move(other.frame_)), type_(other.type_){}
	
	producer_frame(drawable_frame_ptr&& frame) : frame_(std::move(frame)), type_(normal_type){}
	producer_frame(write_frame&& frame) : frame_(std::make_shared<write_frame>(std::move(frame))), type_(normal_type){}
	producer_frame(composite_frame&& frame) : frame_(std::make_shared<composite_frame>(std::move(frame))), type_(normal_type){}
	producer_frame(transform_frame&& frame) : frame_(std::make_shared<transform_frame>(std::move(frame))), type_(normal_type){}

	producer_frame(eof_frame&&) : type_(eof_type){}
	producer_frame(empty_frame&&) : type_(empty_type){}
	
	const std::vector<short>& audio_data() const 
	{
		static std::vector<short> no_audio;
		return frame_ ? frame_->audio_data() : no_audio;
	}	

	void begin_write()
	{
		if(frame_)
			frame_->begin_write();
	}

	void end_write()
	{
		if(frame_)
			frame_->end_write();
	}

	void draw(frame_shader& shader)
	{
		if(frame_)
			frame_->draw(shader);
	}

	void swap(producer_frame& other)
	{
		frame_.swap(other.frame_);
		std::swap(type_, other.type_);
	}

	static eof_frame eof(){return eof_frame();}
	static empty_frame empty(){return empty_frame();}
	
	bool operator==(const eof_frame&){return type_ == eof_type;}
	bool operator!=(const eof_frame&){return type_ != eof_type;}

	bool operator==(const empty_frame&){return type_ == empty_type;}
	bool operator!=(const empty_frame&){return type_ != empty_type;}

	bool operator==(const producer_frame& other){return frame_ == other.frame_ && type_ == other.type_;}
	bool operator!=(const producer_frame& other){return !(*this == other);}
	
	producer_frame& operator=(const producer_frame& other)
	{
		producer_frame temp(other);
		temp.swap(*this);
		return *this;
	}
	producer_frame& operator=(producer_frame&& other)
	{
		frame_ = std::move(other.frame_);
		std::swap(type_, other.type_);
		return *this;
	}
	producer_frame& operator=(eof_frame&& frame)
	{
		frame_ = nullptr;
		type_ = eof_type;
		return *this;
	}	
	producer_frame& operator=(empty_frame&& frame)
	{
		frame_ = nullptr;
		type_ = empty_type;
		return *this;
	}	

private:		
	drawable_frame_ptr frame_;
	frame_type type_;
};

}}