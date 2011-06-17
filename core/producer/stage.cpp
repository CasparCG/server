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

#include "stage.h"

#include "../video_channel_context.h"

#include "layer.h"

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>

#include <common/concurrency/executor.h>

#include <boost/range/algorithm.hpp>

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

struct stage::implementation : boost::noncopyable
{		
	std::map<int, layer>						layers_;		
	typedef std::map<int, layer>::value_type	layer_t;
		
	video_channel_context&						channel_;
public:
	implementation(video_channel_context& video_channel)  
		: channel_(video_channel)
	{
	}
						
	std::map<int, safe_ptr<basic_frame>> execute()
	{			
		std::map<int, safe_ptr<basic_frame>> frames;

		try
		{
			// Allocate placeholders.
			BOOST_FOREACH(auto layer, layers_)
				frames[layer.first] = basic_frame::empty();

			// Render layers
			tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](layer_t& layer)
			{
				frames[layer.first] = layer.second.receive();
			});
		}
		catch(...)
		{
			CASPAR_LOG_CURRENT_EXCEPTION();
		}
		
		return frames;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool preview)
	{
		channel_.execution().invoke([&]
		{
			layers_[index].load(make_safe<destroy_producer_proxy>(channel_.destruction(), producer), preview);
		});
	}

	void pause(int index)
	{		
		channel_.execution().invoke([&]
		{
			layers_[index].pause();
		});
	}

	void play(int index)
	{		
		channel_.execution().invoke([&]
		{
			layers_[index].play();
		});
	}

	void stop(int index)
	{		
		channel_.execution().invoke([&]
		{
			layers_[index].stop();
		});
	}

	void clear(int index)
	{
		channel_.execution().invoke([&]
		{
			layers_.erase(index);
		});
	}
		
	void clear()
	{
		channel_.execution().invoke([&]
		{
			layers_.clear();
		});
	}	
	
	void swap_layer(int index, size_t other_index)
	{
		channel_.execution().invoke([&]
		{
			layers_[index].swap(layers_[other_index]);
		});
	}

	void swap_layer(int index, size_t other_index, stage& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			if(channel_.get_format_desc() != other.impl_->channel_.get_format_desc())
				BOOST_THROW_EXCEPTION(not_supported() << msg_info("Cannot swap between channels with different formats."));

			auto func = [&]{layers_[index].swap(other.impl_->layers_[other_index]);};
		
			channel_.execution().invoke([&]{other.impl_->channel_.execution().invoke(func);});
		}
	}

	void swap(stage& other)
	{
		if(other.impl_.get() == this)
			return;

		if(channel_.get_format_desc() != other.impl_->channel_.get_format_desc())
			BOOST_THROW_EXCEPTION(not_supported() << msg_info("Cannot swap between channels with different formats."));

		auto func = [&]
		{
			auto sel_first = [](const std::pair<int, layer>& pair){return pair.first;};

			std::set<int> indices;
			auto inserter = std::inserter(indices, indices.begin());

			std::transform(layers_.begin(), layers_.end(), inserter, sel_first);
			std::transform(other.impl_->layers_.begin(), other.impl_->layers_.end(), inserter, sel_first);

			BOOST_FOREACH(auto index, indices)
				layers_[index].swap(other.impl_->layers_[index]);
		};
		
		channel_.execution().invoke([&]{other.impl_->channel_.execution().invoke(func);});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index)
	{
		return channel_.execution().begin_invoke([=]{return layers_[index].foreground();});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> background(int index)
	{
		return channel_.execution().begin_invoke([=]{return layers_[index].background();});
	}

	std::wstring print() const
	{
		return L"stage [" + boost::lexical_cast<std::wstring>(channel_.index()) + L"]";
	}

};

stage::stage(video_channel_context& video_channel) : impl_(new implementation(video_channel)){}
void stage::swap(stage& other){impl_->swap(other);}
void stage::load(int index, const safe_ptr<frame_producer>& producer, bool preview){impl_->load(index, producer, preview);}
void stage::pause(int index){impl_->pause(index);}
void stage::play(int index){impl_->play(index);}
void stage::stop(int index){impl_->stop(index);}
void stage::clear(int index){impl_->clear(index);}
void stage::clear(){impl_->clear();}
void stage::swap_layer(int index, size_t other_index){impl_->swap_layer(index, other_index);}
void stage::swap_layer(int index, size_t other_index, stage& other){impl_->swap_layer(index, other_index, other);}
boost::unique_future<safe_ptr<frame_producer>> stage::foreground(size_t index) {return impl_->foreground(index);}
boost::unique_future<safe_ptr<frame_producer>> stage::background(size_t index) {return impl_->background(index);}
std::map<int, safe_ptr<basic_frame>> stage::execute(){return impl_->execute();}
}}