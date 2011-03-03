#include "StdAfx.h"

#include "frame_mixer_device.h"

#include "audio/audio_mixer.h"
#include "audio/audio_transform.h"
#include "frame/write_frame.h"
#include "frame/read_frame.h"
#include "image/image_mixer.h"
#include "image/image_transform.h"

#include <common/exception/exceptions.h>
#include <common/concurrency/executor.h>
#include <common/diagnostics/graph.h>
#include <common/utility/assert.h>
#include <common/utility/timer.h>
#include <common/gl/gl_check.h>

#include <core/video_format.h>

#include <tbb/concurrent_queue.h>
#include <tbb/concurrent_unordered_map.h>

#include <boost/range/algorithm.hpp>

#include <unordered_map>

namespace caspar { namespace core {
	
	
template<typename T>
class basic_animated_value
{
	T source_;
	T dest_;
	int duration_;
	int time_;
public:	
	basic_animated_value()
		: duration_(0)
		, time_(0){}
	basic_animated_value(const T& dest)
		: source_(dest)
		, dest_(dest)
		, duration_(0)
		, time_(0){}

	basic_animated_value(const T& source, const T& dest, int duration)
		: source_(source)
		, dest_(dest)
		, duration_(duration)
		, time_(0){}
	
	virtual T fetch()
	{
		return lerp(source_, dest_, duration_ < 1 ? 1.0f : static_cast<float>(time_)/static_cast<float>(duration_));
	}
	virtual T fetch_and_tick(int num)
	{						
		time_ = std::min(time_+num, duration_);
		return fetch();
	}
};

struct frame_mixer_device::implementation : boost::noncopyable
{		
	const printer			parent_printer_;
	const video_format_desc format_desc_;

	safe_ptr<diagnostics::graph> graph_;
	timer perf_timer_;
	timer wait_perf_timer_;

	audio_mixer	audio_mixer_;
	image_mixer image_mixer_;

	output_func output_;

	std::unordered_map<int, basic_animated_value<image_transform>> image_transforms_;
	std::unordered_map<int, basic_animated_value<audio_transform>> audio_transforms_;

	basic_animated_value<image_transform> root_image_transform_;
	basic_animated_value<audio_transform> root_audio_transform_;

	executor executor_;
public:
	implementation(const printer& parent_printer, const video_format_desc& format_desc, const output_func& output) 
		: parent_printer_(parent_printer)
		, format_desc_(format_desc)
		, graph_(diagnostics::create_graph(narrow(print())))
		, image_mixer_(format_desc)
		, output_(output)
		, executor_(print())
	{
		graph_->guide("frame-time", 0.5f);	
		graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
		graph_->set_color("tick-time", diagnostics::color(0.1f, 0.7f, 0.8f));
		graph_->set_color("input-buffer", diagnostics::color(1.0f, 1.0f, 0.0f));		
		executor_.start();
		executor_.set_capacity(2);
		CASPAR_LOG(info) << print() << L" Successfully initialized.";	
	}
		
	void send(const std::vector<safe_ptr<draw_frame>>& frames)
	{			
		executor_.begin_invoke([=]
		{
			perf_timer_.reset();
			auto image = image_mixer_.begin_pass();
			BOOST_FOREACH(auto& frame, frames)
			{
				if(format_desc_.mode != video_mode::progressive)
				{
					auto frame1 = make_safe<draw_frame>(frame);
					auto frame2 = make_safe<draw_frame>(frame);

					frame1->get_image_transform() = root_image_transform_.fetch_and_tick(1)*image_transforms_[frame->get_layer_index()].fetch_and_tick(1);
					frame2->get_image_transform() = root_image_transform_.fetch_and_tick(1)*image_transforms_[frame->get_layer_index()].fetch_and_tick(1);

					if(frame1->get_image_transform() != frame2->get_image_transform())
						draw_frame::interlace(frame1, frame2, format_desc_.mode)->process_image(image_mixer_);
					else
						frame2->process_image(image_mixer_);
				}
				else
				{
					auto frame1 = make_safe<draw_frame>(frame);
					frame1->get_image_transform() = root_image_transform_.fetch_and_tick(1)*image_transforms_[frame->get_layer_index()].fetch_and_tick(1);
					frame1->process_image(image_mixer_);
				}
			}
			image_mixer_.end_pass();

			auto audio = audio_mixer_.begin_pass();
			BOOST_FOREACH(auto& frame, frames)
			{
				int num = format_desc_.mode == video_mode::progressive ? 1 : 2;
				auto transform = root_audio_transform_.fetch_and_tick(num)*audio_transforms_[frame->get_layer_index()].fetch_and_tick(num);
				audio_mixer_.begin(transform);
				frame->process_audio(audio_mixer_);
				audio_mixer_.end();
			}
			audio_mixer_.end_pass();

			graph_->update("frame-time", static_cast<float>(perf_timer_.elapsed()/format_desc_.interval*0.5));

			output_(make_safe<const read_frame>(std::move(image.get()), std::move(audio)));
			graph_->update("tick-time", static_cast<float>(wait_perf_timer_.elapsed()/format_desc_.interval*0.5));
			wait_perf_timer_.reset();

			graph_->set("input-buffer", static_cast<float>(executor_.size())/static_cast<float>(executor_.capacity()));
		});
		graph_->set("input-buffer", static_cast<float>(executor_.size())/static_cast<float>(executor_.capacity()));
	}
		
	safe_ptr<write_frame> create_frame(const pixel_format_desc& desc)
	{
		return make_safe<write_frame>(desc, image_mixer_.create_buffers(desc));
	}
				
	void set_image_transform(const image_transform& transform, int)
	{
		executor_.invoke([&]
		{
			root_image_transform_ = transform;
		});
	}

	void set_audio_transform(const audio_transform& transform, int)
	{
		executor_.invoke([&]
		{
			root_audio_transform_ = transform;
		});
	}

	void set_image_transform(int index, const image_transform& transform, int)
	{
		executor_.invoke([&]
		{
			image_transforms_[index] = transform;
		});
	}

	void set_audio_transform(int index, const audio_transform& transform, int)
	{
		executor_.invoke([&]
		{
			audio_transforms_[index] = transform;
		});
	}
	
	void apply_image_transform(const std::function<image_transform(const image_transform&)>& transform, int mix_duration)
	{
		return executor_.invoke([&]
		{
			auto src = root_image_transform_.fetch();
			auto dst = transform(src);
			root_image_transform_ = basic_animated_value<image_transform>(src, dst, mix_duration);
		});
	}

	void apply_audio_transform(const std::function<audio_transform(audio_transform)>& transform, int mix_duration)
	{
		return executor_.invoke([&]
		{
			auto src = root_audio_transform_.fetch();
			auto dst = transform(src);
			root_audio_transform_ = basic_animated_value<audio_transform>(src, dst, mix_duration);
		});
	}

	void apply_image_transform(int index, const std::function<image_transform(image_transform)>& transform, int mix_duration)
	{
		executor_.invoke([&]
		{
			auto src = image_transforms_[index].fetch();
			auto dst = transform(src);
			image_transforms_[index] = basic_animated_value<image_transform>(src, dst, mix_duration);
		});
	}

	void apply_audio_transform(int index, const std::function<audio_transform(audio_transform)>& transform, int mix_duration)
	{
		executor_.invoke([&]
		{
			auto src = audio_transforms_[index].fetch();
			auto dst = transform(src);
			audio_transforms_[index] = basic_animated_value<audio_transform>(src, dst, mix_duration);
		});
	}

	std::wstring print() const
	{
		return (parent_printer_ ? parent_printer_() + L"/" : L"") + L"mixer";
	}
};
	
frame_mixer_device::frame_mixer_device(const printer& parent_printer, const video_format_desc& format_desc, const output_func& output) : impl_(new implementation(parent_printer, format_desc, output)){}
frame_mixer_device::frame_mixer_device(frame_mixer_device&& other) : impl_(std::move(other.impl_)){}
void frame_mixer_device::send(const std::vector<safe_ptr<draw_frame>>& frames){impl_->send(frames);}
const video_format_desc& frame_mixer_device::get_video_format_desc() const { return impl_->format_desc_; }
safe_ptr<write_frame> frame_mixer_device::create_frame(const pixel_format_desc& desc){ return impl_->create_frame(desc); }		
safe_ptr<write_frame> frame_mixer_device::create_frame(size_t width, size_t height, pixel_format::type pix_fmt)
{
	// Create bgra frame
	pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back(pixel_format_desc::plane(width, height, 4));
	return create_frame(desc);
}
			
safe_ptr<write_frame> frame_mixer_device::create_frame(pixel_format::type pix_fmt)
{
	// Create bgra frame with output resolution
	pixel_format_desc desc;
	desc.pix_fmt = pix_fmt;
	desc.planes.push_back(pixel_format_desc::plane(get_video_format_desc().width, get_video_format_desc().height, 4));
	return create_frame(desc);
}
void frame_mixer_device::set_image_transform(const image_transform& transform, int mix_duration){impl_->set_image_transform(transform, mix_duration);}
void frame_mixer_device::set_image_transform(int index, const image_transform& transform, int mix_duration){impl_->set_image_transform(index, transform, mix_duration);}
void frame_mixer_device::set_audio_transform(const audio_transform& transform, int mix_duration){impl_->set_audio_transform(transform, mix_duration);}
void frame_mixer_device::set_audio_transform(int index, const audio_transform& transform, int mix_duration){impl_->set_audio_transform(index, transform, mix_duration);}
void frame_mixer_device::apply_image_transform(const std::function<image_transform(image_transform)>& transform, int mix_duration ){impl_->apply_image_transform(transform, mix_duration);}
void frame_mixer_device::apply_image_transform(int index, const std::function<image_transform(image_transform)>& transform, int mix_duration){impl_->apply_image_transform(index, transform, mix_duration);}
void frame_mixer_device::apply_audio_transform(const std::function<audio_transform(audio_transform)>& transform, int mix_duration){impl_->apply_audio_transform(transform, mix_duration);}
void frame_mixer_device::apply_audio_transform(int index, const std::function<audio_transform(audio_transform)>& transform, int mix_duration ){impl_->apply_audio_transform(index, transform, mix_duration);}

}}