#pragma once

#include "write_frame.h"
#include "composite_frame.h"
#include "transform_frame.h"

#include <boost/noncopyable.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/variant.hpp>

#include <memory>
#include <vector>

namespace caspar { namespace core {

class producer_frame
{
public:	
	typedef boost::iterator_range<const unsigned char*> pixel_data_type;
	typedef std::vector<short> audio_data_type;
	
	producer_frame() : frame_(empty_frame()){}
	producer_frame(const write_frame_ptr& frame) : frame_(frame){if(!frame) frame_ = eof_frame();}
	producer_frame(const composite_frame_ptr& frame) : frame_(frame){if(!frame) frame_ = eof_frame();}
	producer_frame(const transform_frame_ptr& frame) : frame_(frame){if(!frame) frame_ = eof_frame();}

	producer_frame(const producer_frame& other) : frame_(other.frame_){}
	producer_frame(producer_frame&& other) : frame_(std::move(other.frame_)){}

	producer_frame& operator=(const producer_frame& other)
	{
		producer_frame temp(other);
		frame_.swap(temp.frame_);
		return *this;
	}

	producer_frame& operator=(producer_frame&& other)
	{
		frame_ = std::move(other.frame_);
		return *this;
	}
		
	const audio_data_type& audio_data() const {return boost::apply_visitor(audio_data_visitor(), frame_);}

	void begin_write() {boost::apply_visitor(begin_write_visitor(), frame_);}	
	void draw(frame_shader& shader) {boost::apply_visitor(draw_visitor(shader), frame_);	}
	void end_write() {boost::apply_visitor(end_write_visitor(), frame_);}
	
	static const producer_frame& eof()
	{
		static producer_frame eof(eof_frame());
		return eof;
	}

	static const producer_frame& empty()
	{
		static producer_frame empty(empty_frame());
		return empty;
	}

	bool operator==(const producer_frame& other)
	{
		return frame_ == other.frame_;
	}

	bool operator!=(const producer_frame& other)
	{
		return !(*this == other);
	}

private:

	friend class frame_processor_device;
	friend class frame_renderer;
	
	struct null_frame
	{
		void begin_write(){}
		void draw(frame_shader& shader){}
		void end_write(){}
		audio_data_type& audio_data() { static audio_data_type audio_data; return audio_data;} 
	};
	typedef std::shared_ptr<null_frame> null_frame_ptr;

	producer_frame(const null_frame_ptr& frame) : frame_(frame){}
		
	static const null_frame_ptr& eof_frame()
	{
		static null_frame_ptr eof(new null_frame());
		return eof;
	}

	static const null_frame_ptr& empty_frame()
	{
		static null_frame_ptr empty(new null_frame());
		return empty;
	}

	struct audio_data_visitor : public boost::static_visitor<audio_data_type&>
	{
		template<typename P> audio_data_type& operator()(P& frame) const{return frame->audio_data();}
	};

	struct begin_write_visitor : public boost::static_visitor<void>
	{
		template<typename P> void operator()(P& frame) const{frame->begin_write();}
	};
	
	struct draw_visitor : public boost::static_visitor<void>
	{
		draw_visitor(frame_shader& shader) : shader_(shader){}
		template<typename P> void operator()(P& frame) const{frame->draw(shader_);}
		frame_shader& shader_;
	};

	struct end_write_visitor : public boost::static_visitor<void>
	{
		template<typename P> void operator()(P& frame) const{frame->end_write();}
	};

	boost::variant<write_frame_ptr, composite_frame_ptr, transform_frame_ptr, null_frame_ptr> frame_;
};

}}