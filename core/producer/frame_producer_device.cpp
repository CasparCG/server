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

#include "../channel_context.h"

#include "layer.h"

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>

#include <common/diagnostics/graph.h>
#include <common/concurrency/executor.h>

#include <boost/timer.hpp>

#include <tbb/parallel_for.h>

#include <map>

namespace caspar { namespace core {
		
void destroy_producer(safe_ptr<frame_producer>& producer)
{
	if(!producer.unique())
		CASPAR_LOG(warning) << producer->print() << L" Not destroyed on safe asynchronous destruction thread.";
		
	producer = frame_producer::empty();
}

class destroy_producer_proxy : public frame_producer
{
	safe_ptr<frame_producer> producer_;
	executor& destroy_context_;
public:
	destroy_producer_proxy(executor& destroy_context, const safe_ptr<frame_producer>& producer) 
		: producer_(producer)
		, destroy_context_(destroy_context){}

	~destroy_producer_proxy()
	{		
		if(destroy_context_.size() > 4)
			CASPAR_LOG(error) << L" Potential destroyer deadlock.";

		destroy_context_.begin_invoke(std::bind(&destroy_producer, std::move(producer_)));
	}

	virtual safe_ptr<basic_frame>		receive()														{return core::receive(producer_);}
	virtual std::wstring				print() const													{return producer_->print();}
	virtual void						param(const std::wstring& str)									{producer_->param(str);}
	virtual safe_ptr<frame_producer>	get_following_producer() const									{return producer_->get_following_producer();}
	virtual void						set_leading_producer(const safe_ptr<frame_producer>& producer)	{producer_->set_leading_producer(producer);}
};

struct frame_producer_device::implementation : boost::noncopyable
{		
	std::map<int, layer>		 layers_;		
	const output_t				 output_;
	
	safe_ptr<diagnostics::graph> diag_;
	boost::timer				 frame_timer_;
	boost::timer				 tick_timer_;
	boost::timer				 output_timer_;
	
	channel_context&			channel_;
public:
	implementation(channel_context& channel, const output_t& output)  
		: diag_(diagnostics::create_graph(std::string("frame_producer_device")))
		, channel_(channel)
		, output_(output)
	{
		diag_->add_guide("frame-time", 0.5f);	
		diag_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		diag_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		diag_->set_color("output-time", diagnostics::color(0.5f, 1.0f, 0.2f));

		channel_.execution.begin_invoke([=]{tick();});		
	}
			
	void tick()
	{				
		try
		{
			auto frame = render();
			output_timer_.restart();
			output_(frame);
			diag_->update_value("output-time", static_cast<float>(output_timer_.elapsed()*channel_.format_desc.fps*0.5));
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}

		channel_.execution.begin_invoke([=]{tick();});
	}
			
	std::map<int, safe_ptr<basic_frame>> render()
	{	
		frame_timer_.restart();

		std::map<int, safe_ptr<basic_frame>> frames;
		BOOST_FOREACH(auto& layer, layers_)
			frames[layer.first] = basic_frame::empty();

		tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](decltype(*layers_.begin())& pair)
		{
			frames[pair.first] = pair.second.receive();
		});
		
		diag_->update_value("frame-time", frame_timer_.elapsed()*channel_.format_desc.fps*0.5);

		diag_->update_value("tick-time", tick_timer_.elapsed()*channel_.format_desc.fps*0.5);
		tick_timer_.restart();

		return frames;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool preview)
	{
		channel_.execution.invoke([&]{layers_[index].load(make_safe<destroy_producer_proxy>(channel_.destruction, producer), preview);});
	}

	void pause(int index)
	{		
		channel_.execution.invoke([&]{layers_[index].pause();});
	}

	void play(int index)
	{		
		channel_.execution.invoke([&]{layers_[index].play();});
	}

	void stop(int index)
	{		
		channel_.execution.invoke([&]{layers_[index].stop();});
	}

	void clear(int index)
	{
		channel_.execution.invoke([&]{layers_.erase(index);});
	}
		
	void clear()
	{
		channel_.execution.invoke([&]{layers_.clear();});
	}	
	
	void swap_layer(int index, size_t other_index)
	{
		channel_.execution.invoke([&]{layers_[index].swap(layers_[other_index]);});
	}

	void swap_layer(int index, size_t other_index, frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			if(channel_.format_desc != other.impl_->channel_.format_desc)
				BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot swap between channels with different formats."));

			auto func = [&]{layers_[index].swap(other.impl_->layers_[other_index]);};
		
			channel_.execution.invoke([&]{other.impl_->channel_.execution.invoke(func);});
		}
	}

	void swap(frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			return;

		if(channel_.format_desc != other.impl_->channel_.format_desc)
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
		
		channel_.execution.invoke([&]{other.impl_->channel_.execution.invoke(func);});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index)
	{
		return channel_.execution.begin_invoke([=]{return layers_[index].foreground();});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> background(int index)
	{
		return channel_.execution.begin_invoke([=]{return layers_[index].background();});
	}
};

frame_producer_device::frame_producer_device(channel_context& channel, const output_t& output)
	: impl_(new implementation(channel, output)){}
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