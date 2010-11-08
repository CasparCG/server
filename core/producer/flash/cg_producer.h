#pragma once

#include "../frame_producer.h"
#include "../../video/video_format.h"
#include "../../channel.h"

namespace caspar { namespace core { namespace flash{
		
class cg_producer : public frame_producer
{
public:
	cg_producer();
	
	frame_ptr render_frame();

	void clear();
	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& start_from_label = TEXT(""), const std::wstring& data = TEXT(""));
	void remove(int layer);
	void play(int layer);
	void stop(int layer, unsigned int mix_out_duration);
	void next(int layer);
	void update(int layer, const std::wstring& data);
	void invoke(int layer, const std::wstring& label);

	const video_format_desc& get_video_format_desc() const;
	void initialize(const frame_processor_device_ptr& frame_processor);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<cg_producer> cg_producer_ptr;

static const unsigned int CG_DEFAULT_LAYER = 5000;

cg_producer_ptr get_default_cg_producer(const channel_ptr& channel, unsigned int layer_index = CG_DEFAULT_LAYER);

}}}