#pragma once

#include "frame_format.h"

#include "gpu_frame_shader.h"
#include "gpu_frame_desc.h"

#include <memory>
#include <array>

#include <boost/noncopyable.hpp>

#include <Glee.h>

#include <boost/tuple/tuple.hpp>

namespace caspar { namespace core {
	
struct rectangle
{
	rectangle(double left, double top, double right, double bottom)
		: left(left), top(top), right(right), bottom(bottom)
	{}
	double left;
	double top;
	double right;
	double bottom;
};

class gpu_frame :  boost::noncopyable
{
public:
	virtual ~gpu_frame(){}
			
	virtual unsigned char* data(size_t index = 0);
	virtual size_t size(size_t index = 0) const;
				
	virtual std::vector<short>& audio_data();

	virtual void alpha(double value);
	virtual void translate(double x, double y);
	virtual void texcoords(const rectangle& texcoords);
	virtual void mode(video_mode mode);	
	virtual void pix_fmt(pixel_format format);
	
	virtual double x() const;
	virtual double y() const;

	static std::shared_ptr<gpu_frame> null()
	{
		static auto my_null_frame = std::shared_ptr<gpu_frame>(new gpu_frame(0,0));
		return my_null_frame;
	}
		
	virtual void begin_write();
	virtual void end_write();
	virtual void begin_read();
	virtual void end_read();
	virtual void draw(const gpu_frame_shader_ptr& shader);

protected:
	gpu_frame(size_t width, size_t height);
	gpu_frame(const gpu_frame_desc& desc);

	friend class gpu_frame_device;

	virtual void reset();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;
	
}}