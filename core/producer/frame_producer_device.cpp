#include "../StdAfx.h"

#include "frame_producer_device.h"

#include "../mixer/frame/draw_frame.h"
#include "../mixer/frame_factory.h"

#include "layer.h"

#include <common/concurrency/executor.h>
#include <common/utility/printable.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/lexical_cast.hpp>

#include <tbb/parallel_for.h>
#include <tbb/mutex.h>

#include <array>
#include <memory>
#include <unordered_map>

namespace caspar { namespace core {

struct frame_producer_device::implementation : boost::noncopyable
{		
	const printer parent_printer_;

	std::unordered_map<int, layer> layers_;		

	output_func output_;

	const safe_ptr<frame_factory> factory_;
	
	mutable executor executor_;
public:
	implementation(const printer& parent_printer, const safe_ptr<frame_factory>& factory, const output_func& output)  
		: parent_printer_(parent_printer)
		, factory_(factory)
		, output_(output)
	{
		executor_.start();
		executor_.begin_invoke([=]{tick();});
	}

	~implementation()
	{
		CASPAR_LOG(info) << "Shutting down producer-device.";
	}
					
	void tick()
	{		
		output_(draw());
		executor_.begin_invoke([=]{tick();});
	}
	
	std::vector<safe_ptr<draw_frame>> draw()
	{	
		std::vector<safe_ptr<draw_frame>> frames(layers_.size(), draw_frame::empty());
		tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size(), 1), [&](const tbb::blocked_range<size_t>& r)
		{
			auto it = layers_.begin();
			std::advance(it, r.begin());
			for(size_t i = r.begin(); i != r.end(); ++i, ++it)
			{
				frames[i] = it->second.receive();
				frames[i]->set_layer_index(i);
			}
		});		
		boost::range::remove_erase(frames, draw_frame::empty());
		return frames;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load)
	{
		producer->set_parent_printer(std::bind(&layer::print, &layers_[index]));
		producer->initialize(factory_);
		executor_.invoke([&]{layers_[index].load(producer, play_on_load);});
	}
			
	void preview(int index, const safe_ptr<frame_producer>& producer)
	{
		producer->initialize(factory_);
		executor_.invoke([&]{layers_[index].preview(producer);});
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
		executor_.invoke([&]{layers_[index].clear();});
	}
		
	void clear()
	{
		executor_.invoke([&]
		{
			BOOST_FOREACH(auto& pair, layers_)
				pair.second.clear();
		});
	}	
	
	void swap_layer(int index, size_t other_index)
	{
		executor_.invoke([&]
		{
			layers_[index].swap(layers_[other_index]);
		});
	}

	void swap_layer(int index, size_t other_index, frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			swap_layer(index, other_index);
		else
		{
			auto func = [&]
			{
				layers_[index].swap(other.impl_->layers_.at(other_index));			
			};
		
			executor_.invoke([&]{other.impl_->executor_.invoke(func);});
		}
	}

	void swap(frame_producer_device& other)
	{
		if(other.impl_.get() == this)
			return;

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
				layers_[index].swap(other.impl_->layers_[index]);
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
		return (parent_printer_ ? parent_printer_() + L"/" : L"") + L"producer";
	}
};

frame_producer_device::frame_producer_device(const printer& parent_printer, const safe_ptr<frame_factory>& factory, const output_func& output) : impl_(new implementation(parent_printer, factory, output)){}
frame_producer_device::frame_producer_device(frame_producer_device&& other) : impl_(std::move(other.impl_)){}
void frame_producer_device::swap(frame_producer_device& other){impl_->swap(other);}
void frame_producer_device::load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load){impl_->load(index, producer, play_on_load);}
void frame_producer_device::preview(int index, const safe_ptr<frame_producer>& producer){impl_->preview(index, producer);}
void frame_producer_device::pause(int index){impl_->pause(index);}
void frame_producer_device::play(int index){impl_->play(index);}
void frame_producer_device::stop(int index){impl_->stop(index);}
void frame_producer_device::clear(int index){impl_->clear(index);}
void frame_producer_device::clear(){impl_->clear();}
void frame_producer_device::swap_layer(int index, size_t other_index){impl_->swap_layer(index, other_index);}
void frame_producer_device::swap_layer(int index, size_t other_index, frame_producer_device& other){impl_->swap_layer(index, other_index, other);}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::foreground(size_t index) const{	return impl_->foreground(index);}
}}