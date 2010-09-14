#pragma once

#include "../frame_producer.h"
#include "../../frame/frame_fwd.h"
#include "../../renderer/render_device.h"

namespace caspar{ namespace flash{
		
class cg_producer : public frame_producer
{
public:
	cg_producer(const frame_format_desc& format_desc, Monitor* pMonitor);
	
	frame_ptr get_frame();

	void clear();
	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& start_from_label = TEXT(""), const std::wstring& data = TEXT(""));
	void remove(int layer);
	void play(int layer);
	void stop(int layer, unsigned int mix_out_duration);
	void next(int layer);
	void update(int layer, const std::wstring& data);
	void invoke(int layer, const std::wstring& label);

	const frame_format_desc& get_frame_format_desc() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};
typedef std::shared_ptr<cg_producer> cg_producer_ptr;

static const unsigned int CG_DEFAULT_LAYER = 5000;

cg_producer_ptr get_default_cg_producer(const renderer::render_device_ptr& render_device, unsigned int layer_index = CG_DEFAULT_LAYER);

}}