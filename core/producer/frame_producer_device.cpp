#include "../StdAfx.h"

#include "frame_producer_device.h"

#include "../mixer/frame/draw_frame.h"
#include "../mixer/frame_factory.h"

#include "layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>
#include <boost/lexical_cast.hpp>

#include <tbb/parallel_for.h>

#include <array>
#include <memory>

namespace caspar { namespace core {

struct frame_producer_device::implementation : boost::noncopyable
{		
	std::array<layer, frame_producer_device::MAX_LAYER> layers_;		

	const output_func output_;
	const safe_ptr<frame_factory> factory_;
	
	mutable executor executor_;

public:
	implementation(const safe_ptr<frame_factory>& factory, const output_func& output)  
		: factory_(factory)
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
		tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size()), 
		[&](const tbb::blocked_range<size_t>& r)
		{
			for(size_t i = r.begin(); i != r.end(); ++i)
				frames[i] = layers_[i].receive();
		});		
		boost::range::remove_erase(frames, draw_frame::eof());
		boost::range::remove_erase(frames, draw_frame::empty());
		return frames;
	}

	void load(size_t index, const safe_ptr<frame_producer>& producer, bool play_on_load)
	{
		check_bounds(index);
		producer->initialize(factory_);
		executor_.invoke([&]
		{
			layers_[index] = layer(index);
			layers_[index].load(producer, play_on_load);
		});
	}
			
	void preview(size_t index, const safe_ptr<frame_producer>& producer)
	{
		check_bounds(index);
		producer->initialize(factory_);
		executor_.invoke([&]
		{			
			layers_[index] = layer(index);
			layers_[index].preview(producer);
		});
	}

	void pause(size_t index)
	{		
		check_bounds(index);
		executor_.invoke([&]
		{
			layers_[index].pause();
		});
	}

	void play(size_t index)
	{		
		check_bounds(index);
		executor_.invoke([&]
		{
			layers_[index].play();
		});
	}

	void stop(size_t index)
	{		
		check_bounds(index);
		executor_.invoke([&]
		{
			layers_[index].stop();
		});
	}

	void clear(size_t index)
	{
		check_bounds(index);
		executor_.invoke([&]
		{
			layers_[index] = std::move(layer());
		});
	}
		
	void clear()
	{
		executor_.invoke([&]
		{
			for(auto it = layers_.begin(); it != layers_.end(); ++it)
				*it = std::move(layer());
		});
	}	
	
	void swap(size_t index, size_t other_index)
	{
		check_bounds(index);
		check_bounds(other_index);

		executor_.invoke([&]
		{
			layers_[index].swap(layers_[other_index]);
		});
	}

	void swap(size_t index, size_t other_index, frame_producer_device& other)
	{
		check_bounds(index);
		check_bounds(other_index);

		executor_.invoke([&]
		{
			layers_[index].swap(other.impl_->layers_[other_index]);
		});
	}

	void check_bounds(size_t index)
	{
		if(index < 0 || index >= frame_producer_device::MAX_LAYER)
			BOOST_THROW_EXCEPTION(out_of_range() << msg_info("Valid range is [0..100]") << arg_name_info("index") << arg_value_info(boost::lexical_cast<std::string>(index)));
	}
	
	boost::unique_future<safe_ptr<frame_producer>> foreground(size_t index) const
	{
		return executor_.begin_invoke([=]() -> safe_ptr<frame_producer>
		{			
			return layers_[index].foreground();
		});
	}
};

frame_producer_device::frame_producer_device(const safe_ptr<frame_factory>& factory, const output_func& output) : impl_(new implementation(factory, output)){}
frame_producer_device::frame_producer_device(frame_producer_device&& other) : impl_(std::move(other.impl_)){}
void frame_producer_device::load(size_t index, const safe_ptr<frame_producer>& producer, bool play_on_load){impl_->load(index, producer, play_on_load);}
void frame_producer_device::preview(size_t index, const safe_ptr<frame_producer>& producer){impl_->preview(index, producer);}
void frame_producer_device::pause(size_t index){impl_->pause(index);}
void frame_producer_device::play(size_t index){impl_->play(index);}
void frame_producer_device::stop(size_t index){impl_->stop(index);}
void frame_producer_device::clear(size_t index){impl_->clear(index);}
void frame_producer_device::clear(){impl_->clear();}
void frame_producer_device::swap(size_t index, size_t other_index){impl_->swap(index, other_index);}
void frame_producer_device::swap(size_t index, size_t other_index, frame_producer_device& other){impl_->swap(index, other_index, other);}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::foreground(size_t index) const{	return impl_->foreground(index);}
}}