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

#include "../StdAfx.h"

#include "frame_producer_device.h"
#include "destroy_producer_proxy.h"

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>

#include <common/diagnostics/graph.h>

#include "layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/timer.hpp>

#include <tbb/parallel_for.h>
#include <tbb/mutex.h>
#include <tbb/combinable.h>

#include <array>
#include <memory>
#include <map>

namespace caspar { namespace core {

struct frame_producer_device::implementation : boost::noncopyable
{		
	std::map<int, layer> layers_;		
	
	const video_format_desc format_desc_;
	
	safe_ptr<diagnostics::graph> diag_;

	output_t output_;

	boost::timer frame_timer_;
	boost::timer tick_timer_;
	boost::timer output_timer_;
	
	mutable executor executor_;
public:
	implementation(const video_format_desc& format_desc)  
		: format_desc_(format_desc)
		, diag_(diagnostics::create_graph(std::string("frame_producer_device")))
		, executor_(L"frame_producer_device")
	{
		diag_->add_guide("frame-time", 0.5f);	
		diag_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		diag_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		diag_->set_color("output-time", diagnostics::color(0.5f, 1.0f, 0.2f));
		executor_.begin_invoke([]
		{
			SetThreadPriority(GetCurrentThread(), ABOVE_NORMAL_PRIORITY_CLASS);
		});
	}

	boost::signals2::connection connect(const output_t::slot_type& subscriber)
	{
		return executor_.invoke([&]() -> boost::signals2::connection
		{
			if(output_.empty())
				executor_.begin_invoke([=]{tick();});		
			return output_.connect(subscriber);
		});
	}
					
	void tick()
	{				
		if(output_.empty())
			return;				
		
		auto frame = draw();
		output_timer_.restart();
		output_(frame);
		diag_->update_value("output-time", static_cast<float>(output_timer_.elapsed()*format_desc_.fps*0.5));

		executor_.begin_invoke([=]{tick();});
	}
			
	std::map<int, safe_ptr<basic_frame>> draw()
	{	
		frame_timer_.restart();

		tbb::combinable<std::map<int, safe_ptr<basic_frame>>> frames;

		tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](decltype(*layers_.begin())& pair)
		{
			auto frame = pair.second.receive();
			if(is_concrete_frame(frame))
				frames.local()[pair.first] = frame;		
		});

		std::map<int, safe_ptr<basic_frame>> result;
		frames.combine_each([&](const std::map<int, safe_ptr<basic_frame>>& map)
		{
			result.insert(map.begin(), map.end());
		});

		diag_->update_value("frame-time", static_cast<float>(frame_timer_.elapsed()*format_desc_.fps*0.5));
				
		diag_->update_value("tick-time", static_cast<float>(tick_timer_.elapsed()*format_desc_.fps*0.5));
		tick_timer_.restart();

		return result;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool preview)
	{
		executor_.invoke([&]{layers_[index].load(make_safe<destroy_producer_proxy>(producer), preview);});
	}

	void pause(int index)
	{		
		executor_.invoke([&]{layers_[index].pause();});
	}

	void play(int index)
	{		
		executor_.invoke([&]{layers_[index].play();});
	}

	void stop(int index)
	{		
		executor_.invoke([&]{layers_[index].stop();});
	}

	void clear(int index)
	{
		executor_.invoke([&]{layers_.erase(index);});
	}
		
	void clear()
	{
		executor_.invoke([&]{layers_.clear();});
	}	
	
	void swap_layer(int index, size_t other_index)
	{
		executor_.invoke([&]{layers_[index].swap(layers_[other_index]);});
	}

	void swap_layer(int index, size_t other_index, frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			if(format_desc_ != other.impl_->format_desc_)
				BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot swap between channels with different formats."));

			auto func = [&]{layers_[index].swap(other.impl_->layers_[other_index]);};
		
			executor_.invoke([&]{other.impl_->executor_.invoke(func);});
		}
	}

	void swap(frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			return;

		if(format_desc_ != other.impl_->format_desc_)
			BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot swap between channels with different formats."));

		auto func = [&]
		{
			auto sel_first = [](const std::pair<int, layer>& pair){return pair.first;};

			std::set<int> indices;
			auto inserter = std::inserter(indices, indices.begin());

			std::transform(layers_.begin(), layers_.end(), inserter, sel_first);
			std::transform(other.impl_->layers_.begin(), other.impl_->layers_.end(), inserter, sel_first);
			std::for_each(indices.begin(), indices.end(), [&](int index)
			{
				layers_[index].swap(other.impl_->layers_[index]);
			});					
		};
		
		executor_.invoke([&]{other.impl_->executor_.invoke(func);});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index)
	{
		return executor_.begin_invoke([=]{return layers_[index].foreground();});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> background(int index)
	{
		return executor_.begin_invoke([=]{return layers_[index].background();});
	}
};

frame_producer_device::frame_producer_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
frame_producer_device::frame_producer_device(frame_producer_device&& other) : impl_(std::move(other.impl_)){}
boost::signals2::connection frame_producer_device::connect(const output_t::slot_type& subscriber){return impl_->connect(subscriber);}
void frame_producer_device::swap(frame_producer_device& other){impl_->swap(other);}
void frame_producer_device::load(int index, const safe_ptr<frame_producer>& producer, bool preview){impl_->load(index, producer, preview);}
void frame_producer_device::pause(int index){impl_->pause(index);}
void frame_producer_device::play(int index){impl_->play(index);}
void frame_producer_device::stop(int index){impl_->stop(index);}
void frame_producer_device::clear(int index){impl_->clear(index);}
void frame_producer_device::clear(){impl_->clear();}
void frame_producer_device::swap_layer(int index, size_t other_index){impl_->swap_layer(index, other_index);}
void frame_producer_device::swap_layer(int index, size_t other_index, frame_producer_device& other){impl_->swap_layer(index, other_index, other);}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::foreground(size_t index) {return impl_->foreground(index);}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::background(size_t index) {return impl_->background(index);}
}}