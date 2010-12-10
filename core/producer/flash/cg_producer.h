#pragma once

#include "../frame_producer.h"
#include "../../format/video_format.h"
#include "../../channel.h"

namespace caspar { namespace core { namespace flash{
		
class cg_producer : public frame_producer
{
public:
	static const unsigned int DEFAULT_LAYER = 5000;

	cg_producer();
	
	virtual gpu_frame_ptr receive();
	virtual void initialize(const frame_processor_device_ptr& frame_processor);

	void clear();
	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& start_from_label = TEXT(""), const std::wstring& data = TEXT(""));
	void remove(int layer);
	void play(int layer);
	void stop(int layer, unsigned int mix_out_duration);
	void next(int layer);
	void update(int layer, const std::wstring& data);
	void invoke(int layer, const std::wstring& label);

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<cg_producer> cg_producer_ptr;

cg_producer_ptr get_default_cg_producer(const channel_ptr& channel, int layer_index = cg_producer::DEFAULT_LAYER);

}}}