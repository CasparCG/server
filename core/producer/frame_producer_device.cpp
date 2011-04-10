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

	~implementation()
	{
		CASPAR_LOG(info) << "Shutting down producer-device.";
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
		
	layer& get_layer(int index)
	{
		auto it = layers_.find(index);
		if(it == layers_.end())
			it = layers_.insert(std::make_pair(index, layer(index))).first;
		return it->second;
	}
	
	std::vector<safe_ptr<basic_frame>> draw()
	{	
		std::vector<safe_ptr<basic_frame>> frames(layers_.size(), basic_frame::empty());
		tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size(), 1), [&](const tbb::blocked_range<size_t>& r)
		{
			auto it = layers_.begin();
			std::advance(it, r.begin());
			for(size_t i = r.begin(); i != r.end(); ++i, ++it)
			{
				frames[i] = it->second.receive();
				frames[i]->set_layer_index(it->first);
			}
		});		
		return frames;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load, bool preview)
	{
		executor_.invoke([&]{get_layer(index).load(producer, play_on_load, preview);});
	}

	void pause(int index)
	{		
		executor_.invoke([&]{get_layer(index).pause();});
	}

	void play(int index)
	{		
		executor_.invoke([&]{get_layer(index).play();});
	}

	void stop(int index)
	{		
		executor_.invoke([&]{get_layer(index).stop();});
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
		executor_.invoke([&]
		{
			get_layer(index).swap(layers_[other_index]);
		});
	}

	void swap_layer(int index, size_t other_index, frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			if(format_desc_ != other.impl_->format_desc_)
				BOOST_THROW_EXCEPTION(invalid_operation() << msg_info("Cannot swap between channels with different formats."));

			auto func = [&]
			{
				get_layer(index).swap(other.impl_->layers_.at(other_index));		

				CASPAR_LOG(info) << print() << L" Swapped layer " << index << L" with " << other.impl_->print() << L" layer " << other_index << L".";	
			};
		
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
			std::set<int> my_indices;
			BOOST_FOREACH(auto& pair, layers_)
				my_indices.insert(pair.first);

			std::set<int> other_indicies;
			BOOST_FOREACH(auto& pair, other.impl_->layers_)
				other_indicies.insert(pair.first);
			
			std::vector<int> indices;
			std::set_union(my_indices.begin(), my_indices.end(), other_indicies.begin(), other_indicies.end(), std::back_inserter(indices));
			
			BOOST_FOREACH(auto index, indices)
				get_layer(index).swap(other.impl_->get_layer(index));

			CASPAR_LOG(info) << print() << L" Swapped layers with " << other.impl_->print() << L".";
		};
		
		executor_.invoke([&]{other.impl_->executor_.invoke(func);});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(int index) const
	{
		return executor_.begin_invoke([=]() mutable -> safe_ptr<frame_producer>
		{			
			auto it = layers_.find(index);
			return it != layers_.end() ? it->second.foreground() : frame_producer::empty();
		});
	}

	std::wstring print() const
	{
		return L"frame_producer_device";
	}
};

frame_producer_device::frame_producer_device(const video_format_desc& format_desc) : impl_(new implementation(format_desc)){}
frame_producer_device::frame_producer_device(frame_producer_device&& other) : impl_(std::move(other.impl_)){}
boost::signals2::connection frame_producer_device::connect(const output_t::slot_type& subscriber){return impl_->connect(subscriber);}
void frame_producer_device::swap(frame_producer_device& other){impl_->swap(other);}
void frame_producer_device::load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load, bool preview){impl_->load(index, producer, play_on_load, preview);}
void frame_producer_device::pause(int index){impl_->pause(index);}
void frame_producer_device::play(int index){impl_->play(index);}
void frame_producer_device::stop(int index){impl_->stop(index);}
void frame_producer_device::clear(int index){impl_->clear(index);}
void frame_producer_device::clear(){impl_->clear();}
void frame_producer_device::swap_layer(int index, size_t other_index){impl_->swap_layer(index, other_index);}
void frame_producer_device::swap_layer(int index, size_t other_index, frame_producer_device& other){impl_->swap_layer(index, other_index, other);}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::foreground(size_t index) const{	return impl_->foreground(index);}
}}