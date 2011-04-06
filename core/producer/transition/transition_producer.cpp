/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/ 
#include "../../stdafx.h"

#include "transition_producer.h"

#include <core/video_format.h>

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/image_transform.h>
#include <core/producer/frame/audio_transform.h>

namespace caspar { namespace core {	

struct transition_producer::implementation : boost::noncopyable
{	
	const transition_producer* self_;
	unsigned short				current_frame_;
	
	const transition_info		info_;
	
	safe_ptr<frame_producer>	dest_producer_;
	safe_ptr<frame_producer>	source_producer_;

	std::shared_ptr<frame_factory>	frame_factory_;

	std::vector<safe_ptr<basic_frame>> frame_buffer_;
	
	implementation(const transition_producer* self, const safe_ptr<frame_producer>& dest, const transition_info& info) 
		: self_(self)
		, current_frame_(0)
		, info_(info)
		, dest_producer_(dest)
		, source_producer_(frame_producer::empty())
	{
		dest_producer_->set_parent(self_);
		frame_buffer_.push_back(basic_frame::empty());
	}
				
	void set_frame_factory(const safe_ptr<frame_factory>& frame_factory)
	{
		dest_producer_->set_frame_factory(frame_factory);
		frame_factory_ = frame_factory;
	}
	
	safe_ptr<frame_producer> get_following_producer() const
	{
		return dest_producer_;
	}
	
	void set_leading_producer(const safe_ptr<frame_producer>& producer)
	{
		source_producer_ = producer;
		source_producer_->set_parent(self_);
	}

	safe_ptr<basic_frame> receive()
	{
		if(current_frame_++ >= info_.duration)
			return basic_frame::eof();

		auto source = basic_frame::empty();
		auto dest = basic_frame::empty();

		tbb::parallel_invoke
		(
			[&]{dest   = render_sub_frame(dest_producer_);},
			[&]{source = render_sub_frame(source_producer_);}
		);

		return compose(dest, source);
	}
	
	safe_ptr<basic_frame> render_sub_frame(safe_ptr<frame_producer>& producer)
	{
		if(producer == frame_producer::empty())
			return basic_frame::eof();

		auto frame = basic_frame::eof();
		try
		{
			frame = producer->receive();
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
			producer = frame_producer::empty();
			CASPAR_LOG(warning) << self_->print() << " Failed to receive frame. Removed producer from transition.";
		}

		if(frame == basic_frame::eof())
		{
			try
			{
				auto following = producer->get_following_producer();
				following->set_frame_factory(safe_ptr<frame_factory>(frame_factory_));
				following->set_leading_producer(producer);
				following->set_parent(self_);
				producer = std::move(following);
			}
			catch(...)
			{
				CASPAR_LOG_CURRENT_EXCEPTION();
				producer = frame_producer::empty();
				CASPAR_LOG(warning) << self_->print() << " Failed to initialize following producer.";
			}

			return render_sub_frame(producer);
		}
		return frame;
	}
					
	safe_ptr<basic_frame> compose(const safe_ptr<basic_frame>& dest_frame, const safe_ptr<basic_frame>& src_frame) 
	{	
		if(dest_frame == basic_frame::eof() && src_frame == basic_frame::eof())
			return basic_frame::eof();

		if(info_.type == transition::cut)		
			return src_frame != basic_frame::eof() ? src_frame : basic_frame::empty();
										
		double delta1 = info_.tweener(current_frame_*2-1, 0.0, 1.0, info_.duration*2);
		double delta2 = info_.tweener(current_frame_*2, 0.0, 1.0, info_.duration*2);  

		double dir = info_.direction == transition_direction::from_left ? 1.0 : -1.0;		
		
		// For interlaced transitions. Seperate fields into seperate frames which are transitioned accordingly.
		
		auto s_frame1 = make_safe<basic_frame>(src_frame);
		auto s_frame2 = make_safe<basic_frame>(src_frame);

		s_frame1->get_audio_transform().set_has_audio(false);
		s_frame2->get_audio_transform().set_gain(1.0-delta2);

		auto d_frame1 = make_safe<basic_frame>(dest_frame);
		auto d_frame2 = make_safe<basic_frame>(dest_frame);
		
		d_frame1->get_audio_transform().set_has_audio(false);
		d_frame2->get_audio_transform().set_gain(delta2);

		if(info_.type == transition::mix)
		{
			d_frame1->get_image_transform().set_opacity(delta1);	
			d_frame2->get_image_transform().set_opacity(delta2);	
		}
		else if(info_.type == transition::slide)
		{
			d_frame1->get_image_transform().set_fill_translation((-1.0+delta1)*dir, 0.0);	
			d_frame2->get_image_transform().set_fill_translation((-1.0+delta2)*dir, 0.0);		
		}
		else if(info_.type == transition::push)
		{
			d_frame1->get_image_transform().set_fill_translation((-1.0+delta1)*dir, 0.0);
			d_frame2->get_image_transform().set_fill_translation((-1.0+delta2)*dir, 0.0);

			s_frame1->get_image_transform().set_fill_translation((0.0+delta1)*dir, 0.0);	
			s_frame2->get_image_transform().set_fill_translation((0.0+delta2)*dir, 0.0);		
		}
		else if(info_.type == transition::wipe)		
		{
			d_frame1->get_image_transform().set_key_scale(delta1, 1.0);	
			d_frame2->get_image_transform().set_key_scale(delta2, 1.0);			
		}
		
		auto mode = frame_factory_->get_video_format_desc().mode;
		auto s_frame = s_frame1->get_image_transform() == s_frame2->get_image_transform() ? s_frame2 : basic_frame::interlace(s_frame1, s_frame2, mode);
		auto d_frame = basic_frame::interlace(d_frame1, d_frame2, mode);

		return basic_frame(s_frame, d_frame);
	}

	virtual std::wstring print() const
	{
		return L"transition[" + info_.name() + L":" + boost::lexical_cast<std::wstring>(info_.duration) + L"]";
	}

	std::wstring source_print() const { return print() + L"/source";}
	std::wstring dest_print() const { return print() + L"/dest";}
};

transition_producer::transition_producer(const safe_ptr<frame_producer>& dest, const transition_info& info) : impl_(new implementation(this, dest, info)){}
transition_producer::transition_producer(transition_producer&& other) : impl_(std::move(other.impl_)){}
safe_ptr<basic_frame> transition_producer::receive(){return impl_->receive();}
safe_ptr<frame_producer> transition_producer::get_following_producer() const{return impl_->get_following_producer();}
void transition_producer::set_leading_producer(const safe_ptr<frame_producer>& producer) { impl_->set_leading_producer(producer); }
void transition_producer::set_frame_factory(const safe_ptr<frame_factory>& frame_factory) { impl_->set_frame_factory(frame_factory);}
std::wstring transition_producer::print() const { return frame_producer::print();}// + impl_->print();}

}}

