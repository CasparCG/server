#include "../../StdAfx.h"

#include "frame_processor.h"

#include "../../../common/gpu/pixel_buffer.h"

#include "../frame.h"
#include "../format.h"
#include "../algorithm.h"
#include "../system_frame.h"

#include "../../../common/exception/exceptions.h"

#include <Glee.h>
#include <SFML/Window.hpp>

#include <tbb/concurrent_queue.h>

#include <boost/lexical_cast.hpp>
#include <boost/thread/once.hpp>
#include <boost/thread.hpp>
#include <boost/range.hpp>
#include <boost/foreach.hpp>

#include <functional>
#include <unordered_map>
#include <numeric>

template<typename T>
class buffer_pool
{
public:
	std::shared_ptr<T> create(size_t width, size_t height)
	{
		auto key = width | (height << 16);
		auto& pool = pools_[key];

		if(pool.empty())
			pool.push(std::make_shared<T>(width, height));		

		auto buffer = pool.front();
		pool.pop();

		return std::shared_ptr<T>(buffer.get(), [=](T*){pools_[key].push(buffer);});
	}

private:
	std::unordered_map<size_t, std::queue<std::shared_ptr<T>>> pools_;
};


namespace caspar { namespace gpu {
	
using namespace common::gpu;

class gpu_buffer
{
public:
	gpu_buffer(size_t width, size_t height) : pbo_(width, height), texture_(width, height){}	
	void begin_write(void* src){pbo_.write_to_pbo(src);}
	void end_write(){pbo_.write_to_texture(texture_);}
	void commit(){texture_.draw();}
private:
	pixel_buffer pbo_;
	texture texture_;
};
typedef std::shared_ptr<gpu_buffer> gpu_buffer_ptr;


class frame_buffer
{
public:
	frame_buffer(size_t width, size_t height) : texture_(width, height)
	{
		glGenFramebuffersEXT(1, &fbo_);
		
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo_);
		glBindTexture(GL_TEXTURE_2D, texture_.handle());
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D, texture_.handle(), 0);
	}

	~frame_buffer()
	{
		glDeleteFramebuffersEXT(1, &fbo_);
	}

	
	GLuint handle() { return fbo_; }
	GLenum attachement() { return GL_COLOR_ATTACHMENT0_EXT; }
	
private:
	common::gpu::texture texture_;
	GLuint fbo_;
};
typedef std::shared_ptr<frame_buffer> frame_buffer_ptr;


struct frame_processor::implementation
{	
	implementation(const frame_format_desc& format_desc) : format_desc_(format_desc)
	{
		queue_.set_capacity(3);
		thread_ = boost::thread([=]{run();});
		empty_frame_ = clear_frame(std::make_shared<system_frame>(format_desc));
	}

	~implementation()
	{
		queue_.push(nullptr);
		finished_frames_.push(nullptr);
		thread_.join();
	}
		
	void composite(const std::vector<frame_ptr>& frames)
	{
		if(frames.empty())
			finished_frames_.push(empty_frame_);
		else
			queue_.push([=]{do_composite(frames);});
	}

	void do_composite(const std::vector<frame_ptr>& frames)
	{		
		index_ = (index_ + 1) % 2;
		int next_index = (index_ + 1) % 2;

		// END WRITE
		boost::range::for_each(input_[index_], std::mem_fn(&gpu_buffer::end_write));
		active_[index_] = input_[index_];	
		
		// BEGIN WRITE
		frame_audio_[next_index].clear();
		std::vector<gpu_buffer_ptr> buffers;
		BOOST_FOREACH(auto frame, frames)
		{
			auto buffer = buffer_pool_.create(frame->width(), frame->height());
			buffer->begin_write(frame->data());
			buffers.push_back(buffer);

			frame_audio_[next_index].insert(frame_audio_[next_index].end(), frame->audio_data().begin(), frame->audio_data().end());
		}
		input_[next_index] = buffers;
				
		// END READ
		auto frame = std::make_shared<system_frame>(format_desc_);
		frame->audio_data() = frame_audio_[index_];
		output_[index_]->read_to_memory(frame->data());
		finished_frames_.push(frame);

		// BEGIN READ		
		glClear(GL_COLOR_BUFFER_BIT);	
		boost::range::for_each(active_[next_index], std::mem_fn(&gpu_buffer::commit));
		output_[next_index]->read_to_pbo(GL_COLOR_ATTACHMENT0_EXT);
	}

	bool try_pop(frame_ptr& frame)
	{
		return finished_frames_.try_pop(frame);
	}
		
	void init()	
	{
		context_.reset(new sf::Context());
		context_->SetActive(true);
		glEnable(GL_TEXTURE_2D);
		glEnable(GL_BLEND);
		glDisable(GL_DEPTH_TEST);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);			
		glClearColor(0.0, 0.0, 0.0, 0.0);
		glViewport(0, 0, format_desc_.width, format_desc_.height);
		glLoadIdentity();

		input_.resize(2);
		active_.resize(2);
		output_.resize(2);
		output_[0] = std::make_shared<pixel_buffer>(format_desc_.width, format_desc_.height);
		output_[1] = std::make_shared<pixel_buffer>(format_desc_.width, format_desc_.height);
		frame_audio_.resize(2);
		fbo_ = std::make_shared<frame_buffer>(format_desc_.width, format_desc_.height);
		index_ = 0;
	}

	void run()
	{
		init();

		std::function<void()> func;
		queue_.pop(func);

		while(func != nullptr)
		{
			func();
			queue_.pop(func);
		}
	}
		
	frame_buffer_ptr fbo_;
	
	int index_;
	std::vector<std::vector<gpu_buffer_ptr>> input_;
	std::vector<std::vector<gpu_buffer_ptr>> active_;

	std::vector<gpu::pixel_buffer_ptr>				output_;

	std::vector<std::vector<audio_chunk_ptr>>		frame_audio_;

	buffer_pool<gpu_buffer> buffer_pool_;
	
	std::unique_ptr<sf::Context> context_;
	boost::thread thread_;
	tbb::concurrent_bounded_queue<std::function<void()>> queue_;
	tbb::concurrent_bounded_queue<frame_ptr> finished_frames_;

	frame_format_desc format_desc_;
	frame_ptr empty_frame_;
};
	
frame_processor::frame_processor(const frame_format_desc& format_desc) : impl_(new implementation(format_desc)){}
void frame_processor::push(const std::vector<frame_ptr>& frames){ impl_->composite(frames);}
bool frame_processor::try_pop(frame_ptr& frame){ return impl_->try_pop(frame);}

}}