#pragma once

#include "../frame_producer.h"
#include "../../video_format.h"
#include "../../channel.h"

namespace caspar { namespace core { namespace flash{
		
class cg_producer : public frame_producer
{
public:
	static const unsigned int DEFAULT_LAYER = 5000;

	explicit cg_producer();
	cg_producer(cg_producer&& other);
	
	virtual safe_ptr<draw_frame> receive();
	virtual void initialize(const safe_ptr<frame_factory>& frame_factory);

	void clear();
	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& start_from_label = TEXT(""), const std::wstring& data = TEXT(""));
	void remove(int layer);
	void play(int layer);
	void stop(int layer, unsigned int mix_out_duration);
	void next(int layer);
	void update(int layer, const std::wstring& data);
	void invoke(int layer, const std::wstring& label);
	
	virtual std::wstring print() const;

private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<channel>& channel, int layer_index = cg_producer::DEFAULT_LAYER);

safe_ptr<frame_producer> create_ct_producer(const std::vector<std::wstring>& params);

}}}