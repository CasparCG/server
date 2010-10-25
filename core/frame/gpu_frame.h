#pragma once

#include <memory>

#include <Glee.h>

namespace caspar {
	
class gpu_frame
{
public:
	gpu_frame(size_t width, size_t height);

	virtual void write_lock();
	virtual bool write_unlock();
	virtual void read_lock(GLenum mode);
	virtual bool read_unlock();
	virtual void draw();
		
	virtual unsigned char* data();
	virtual size_t size() const;
	virtual size_t width() const;
	virtual size_t height() const;
	
	virtual void reset();
			
	virtual const std::vector<short>& audio_data() const;	
	virtual std::vector<short>& audio_data();

	virtual float alpha() const;
	virtual void alpha(float value);

	virtual float x() const;
	virtual float y() const;
	virtual void translate(float x, float y);

	static std::shared_ptr<gpu_frame> null()
	{
		static auto my_null_frame = std::make_shared<gpu_frame>(0,0);
		return my_null_frame;
	}
		
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<gpu_frame> gpu_frame_ptr;
	
inline void mix_audio_safe(std::vector<short>& frame1, const std::vector<short>& frame2)
{
	if(frame1.empty())
		frame1 = std::move(frame2);
	else if(!frame2.empty())
	{
		for(size_t n = 0; n < frame1.size(); ++n)
			frame1[n] = static_cast<short>(static_cast<int>(frame1[n]) + static_cast<int>(frame2[n]) & 0xFFFF);				
	}
}

template<typename frame_ptr_type>
frame_ptr_type& mix_audio_safe(frame_ptr_type& frame1, const gpu_frame_ptr& frame2)
{
	mix_audio_safe(frame1->audio_data(), frame2->audio_data());
	return frame1;
}

}