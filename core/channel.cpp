#include "StdAfx.h"

#include "channel.h"

#include "producer/layer.h"

#include "consumer/frame_consumer_device.h"

#include "processor/draw_frame.h"
#include "processor/frame_processor_device.h"

#include <common/concurrency/executor.h>

#include <boost/range/algorithm_ext/erase.hpp>

#include <tbb/parallel_for.h>

#include <memory>

namespace caspar { namespace core {

class clock
{
public:
	clock(int fps = 25) : fps_(fps)
	{
		QueryPerformanceFrequency(&freq_);
		time_.QuadPart = 0;
	}
		
	// Author: Ryan M. Geiss
	// http://www.geisswerks.com/ryan/FAQS/timing.html
	void wait()
	{     	
		LARGE_INTEGER t;
		QueryPerformanceCounter(&t);

		if (time_.QuadPart != 0)
		{
			int ticks_to_wait = static_cast<int>(freq_.QuadPart / fps_);
			int done = 0;
			do
			{
				QueryPerformanceCounter(&t);
				
				int ticks_passed = static_cast<int>(static_cast<__int64>(t.QuadPart) - static_cast<__int64>(time_.QuadPart));
				int ticks_left = ticks_to_wait - ticks_passed;

				if (t.QuadPart < time_.QuadPart)    // time wrap
					done = 1;
				if (ticks_passed >= ticks_to_wait)
					done = 1;
				
				if (!done)
				{
					// if > 0.002s left, do Sleep(1), which will actually sleep some 
					//   steady amount, probably 1-2 ms,
					//   and do so in a nice way (cpu meter drops; laptop battery spared).
					// otherwise, do a few Sleep(0)'s, which just give up the timeslice,
					//   but don't really save cpu or battery, but do pass a tiny
					//   amount of time.
					if (ticks_left > static_cast<int>((freq_.QuadPart*2)/1000))
						Sleep(1);
					else                        
						for (int i = 0; i < 10; ++i) 
							Sleep(0);  // causes thread to give up its timeslice
				}
			}
			while (!done);            
		}

		time_ = t;
	}
private:
	LARGE_INTEGER freq_;
	LARGE_INTEGER time_;
	int fps_;
};

struct channel::implementation : boost::noncopyable
{	
	implementation(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers)  
		: format_desc_(format_desc), processor_device_(frame_processor_device(format_desc)), consumer_device_(format_desc, consumers)
	{
		executor_.start();
		executor_.begin_invoke([=]{tick();});
	}
					
	void tick()
	{		
		auto drawed_frame = draw();
		auto processed_frame = processor_device_->process(std::move(drawed_frame));
		consumer_device_.consume(std::move(processed_frame));

		executor_.begin_invoke([=]{tick();});

		clock_.wait();
	}
	
	safe_ptr<draw_frame> draw()
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
		return draw_frame(frames);
	}

	void load(int index, const safe_ptr<frame_producer>& producer, bool autoplay)
	{
		producer->initialize(processor_device_);
		executor_.begin_invoke([=]
		{
			auto it = layers_.insert(std::make_pair(index, layer(index))).first;
			it->second.load(producer, autoplay);
		});
	}
			
	void preview(int index, const safe_ptr<frame_producer>& producer)
	{
		producer->initialize(processor_device_);
		executor_.begin_invoke([=]
		{
			auto it = layers_.insert(std::make_pair(index, layer(index))).first;
			it->second.preview(producer);
		});
	}

	void pause(int index)
	{		
		executor_.begin_invoke([=]
		{			
			auto it = layers_.find(index);
			if(it != layers_.end())
				it->second.pause();		
		});
	}

	void play(int index)
	{		
		executor_.begin_invoke([=]
		{
			auto it = layers_.find(index);
			if(it != layers_.end())
				it->second.play();		
		});
	}

	void stop(int index)
	{		
		executor_.begin_invoke([=]
		{
			auto it = layers_.find(index);
			if(it != layers_.end())			
			{
				it->second.stop();	
				if(it->second.empty())
					layers_.erase(it);
			}
		});
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
	}

	clock clock_;

	mutable executor executor_;
				
	safe_ptr<frame_processor_device> processor_device_;
	frame_consumer_device consumer_device_;
						
	std::map<int, layer> layers_;		

	const video_format_desc format_desc_;
};

channel::channel(channel&& other) : impl_(std::move(other.impl_)){}
channel::channel(const video_format_desc& format_desc, const std::vector<safe_ptr<frame_consumer>>& consumers)
	: impl_(new implementation(format_desc, consumers)){}
void channel::load(int index, const safe_ptr<frame_producer>& producer, bool autoplay){impl_->load(index, producer, autoplay);}
void channel::preview(int index, const safe_ptr<frame_producer>& producer){impl_->preview(index, producer);}
void channel::pause(int index){impl_->pause(index);}
void channel::play(int index){impl_->play(index);}
void channel::stop(int index){impl_->stop(index);}
void channel::clear(int index){impl_->clear(index);}
void channel::clear(){impl_->clear();}
boost::unique_future<safe_ptr<frame_producer>> channel::foreground(int index) const{	return impl_->foreground(index);}
boost::unique_future<safe_ptr<frame_producer>> channel::background(int index) const{return impl_->background(index);}
const video_format_desc& channel::get_video_format_desc() const{	return impl_->format_desc_;}

}}