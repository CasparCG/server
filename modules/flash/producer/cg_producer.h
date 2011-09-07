/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include <core/producer/frame_producer.h>
#include <core/producer/stage.h>
#include <core/video_format.h>
#include <core/video_channel.h>

#include <string>

namespace caspar { namespace flash {
		
class cg_producer : public core::frame_producer
{
public:
	static const unsigned int DEFAULT_LAYER = 9999;

	explicit cg_producer(const safe_ptr<core::frame_producer>& producer);
	cg_producer(cg_producer&& other);
	
	// frame_producer
	virtual safe_ptr<core::basic_frame> receive(int);
	virtual safe_ptr<core::basic_frame> last_frame() const;
	virtual std::wstring print() const;

	//cg_producer
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
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<core::video_channel>& video_channel, int layer_index = cg_producer::DEFAULT_LAYER);

safe_ptr<core::frame_producer> create_ct_producer(const safe_ptr<core::frame_factory> frame_factory, const std::vector<std::wstring>& params);

}}