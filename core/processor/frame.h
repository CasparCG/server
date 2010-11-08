#pragma once

#include "frame_shader.h"

#include "../video/video_format.h"
#include "../video/pixel_format.h"

#include <memory>
#include <array>

#include <boost/noncopyable.hpp>

#include <boost/tuple/tuple.hpp>

#include <vector>

namespace caspar { namespace core {
	
class frame :  boost::noncopyable
{
public:
	virtual ~frame(){}
			
	virtual unsigned char* data(size_t index = 0);
	virtual size_t size(size_t index = 0) const;
				
	virtual std::vector<short>& audio_data();

	virtual void alpha(double value);
	virtual void translate(double x, double y);
	virtual void texcoords(double left, double top, double right, double bottom);
	virtual void update_fmt(video_update_format::type fmt);	
	virtual void pix_fmt(pixel_format::type fmt);
	
	virtual double x() const;
	virtual double y() const;

	static std::shared_ptr<frame> empty()
	{
		static auto my_null_frame = std::shared_ptr<frame>(new frame(0,0));
		return my_null_frame;
	}
		
	virtual void begin_write();
	virtual void end_write();
	virtual void begin_read();
	virtual void end_read();
	virtual void draw(const frame_shader_ptr& shader);

protected:
	frame(size_t width, size_t height);
	frame(const pixel_format_desc& desc);

	friend class frame_processor_device;

	virtual void reset();

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<frame> frame_ptr;
	
}}