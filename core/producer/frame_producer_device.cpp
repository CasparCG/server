#include "../StdAfx.h"

#include "frame_producer_device.h"

#include "../mixer/frame/draw_frame.h"
#include "../mixer/frame_factory.h"

#include "layer.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/parallel_for.h>

#include <map>
#include <memory>

namespace caspar { namespace core {

struct frame_producer_device::implementation : boost::noncopyable
{		
	std::map<int, layer> layers_;		

	const video_format_desc format_desc_;	
	const output_func output_;
	const safe_ptr<frame_factory> factory_;
	
	mutable executor executor_;

public:
	implementation(const video_format_desc& format_desc, const safe_ptr<frame_factory>& factory, const output_func& output)  
		: format_desc_(format_desc)
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
		tbb::parallel_for(tbb::blocked_range<size_t>(0, frames.size()), 
		[&](const tbb::blocked_range<size_t>& r)
		{
			auto it = layers_.begin();
			std::advance(it, r.begin());
			for(size_t i = r.begin(); i != r.end(); ++i, ++it)
				frames[i] = it->second.receive();
		});		
		boost::range::remove_erase(frames, draw_frame::eof());
		boost::range::remove_erase(frames, draw_frame::empty());
		return frames;
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load)
	{
		producer->initialize(factory_);
		executor_.begin_invoke([=]
		{
			auto it = layers_.insert(std::make_pair(index, layer(index))).first;
			it->second.load(producer, play_on_load);
		});
	}
			
	void preview(int index, const safe_ptr<frame_producer>& producer)
	{
		producer->initialize(factory_);
		executor_.begin_invoke([=]
		{
			auto it = layers_.insert(std::make_pair(index, layer(index))).first;
			it->second.preview(producer);
		});
	}

	void pause(int index)
	{		
		begin_invoke_layer(index, std::mem_fn(&layer::pause));
	}

	void play(int index)
	{		
		begin_invoke_layer(index, std::mem_fn(&layer::play));
	}

	void stop(int index)
	{		
		begin_invoke_layer(index, std::mem_fn(&layer::stop));
	}

	void clear(int index)
	{
		executor_.begin_invoke([=]
		{			
			auto it = layers_.find(index);
			if(it != layers_.end())
			{
				it->second.clear();		
				layers_.erase(it);
			}
		});
	}
		
	void clear()
	{
		executor_.begin_invoke([=]
		{			
			layers_.clear();
		});
	}		

	template<typename F>
	void begin_invoke_layer(int index, F&& func)
	{
		executor_.begin_invoke([=]
		{
			auto it = layers_.find(index);
			if(it != layers_.end())
				func(it->second);	
		});
	}

	boost::unique_future<safe_ptr<frame_producer>> foreground(int index) const
	{
		return executor_.begin_invoke([=]() -> safe_ptr<frame_producer>
		{			
			auto it = layers_.find(index);
			return it != layers_.end() ? it->second.foreground() : frame_producer::empty();
		});
	}
	
	boost::unique_future<safe_ptr<frame_producer>> background(int index) const
	{
		return executor_.begin_invoke([=]() -> safe_ptr<frame_producer>
		{
			auto it = layers_.find(index);
			return it != layers_.end() ? it->second.background() : frame_producer::empty();
		});
	};
};

frame_producer_device::frame_producer_device(frame_producer_device&& other) : impl_(std::move(other.impl_)){}
frame_producer_device::frame_producer_device(const video_format_desc& format_desc, const safe_ptr<frame_factory>& factory, const output_func& output) 
	: impl_(new implementation(format_desc, factory, output)){}
void frame_producer_device::load(int index, const safe_ptr<frame_producer>& producer, bool play_on_load){impl_->load(index, producer, play_on_load);}
void frame_producer_device::preview(int index, const safe_ptr<frame_producer>& producer){impl_->preview(index, producer);}
void frame_producer_device::pause(int index){impl_->pause(index);}
void frame_producer_device::play(int index){impl_->play(index);}
void frame_producer_device::stop(int index){impl_->stop(index);}
void frame_producer_device::clear(int index){impl_->clear(index);}
void frame_producer_device::clear(){impl_->clear();}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::foreground(int index) const{	return impl_->foreground(index);}
boost::unique_future<safe_ptr<frame_producer>> frame_producer_device::background(int index) const{return impl_->background(index);}
const video_format_desc& frame_producer_device::get_video_format_desc() const{	return impl_->format_desc_;}

}}