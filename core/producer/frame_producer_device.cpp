#include "../StdAfx.h"

#include "frame_producer_device.h"

#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>

#include "layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/lexical_cast.hpp>

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

	output_t output_;
	
	mutable executor executor_;
public:
	implementation(const video_format_desc& format_desc)  
		: format_desc_(format_desc)
		, executor_(L"frame_producer_device")
	{
		executor_.start();
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

		output_(draw());
		executor_.begin_invoke([=]{tick();});
	}
			
	std::map<int, safe_ptr<basic_frame>> draw()
	{	
		tbb::combinable<std::map<int, safe_ptr<basic_frame>>> frames;

		tbb::parallel_for_each(layers_.begin(), layers_.end(), [&](decltype(*layers_.begin())& pair)
		{
			auto frame = pair.second.receive();
			if(frame != basic_frame::empty() && frame != basic_frame::eof())
				frames.local()[pair.first] = frame;		
		});

		std::map<int, safe_ptr<basic_frame>> result;
		frames.combine_each([&](const std::map<int, safe_ptr<basic_frame>>& map)
		{
			result.insert(map.begin(), map.end());
		});

		return result;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool preview)
	{
		executor_.invoke([&]{layers_[index].load(producer, preview);});
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
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::foreground(size_t index) {	return impl_->foreground(index);}
}}