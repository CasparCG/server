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
#include "../../stdafx.h"

#include "playlist_producer.h"

#include <core/producer/frame_producer.h>
#include <core/producer/frame/basic_frame.h>

namespace caspar { namespace core {	

struct playlist_producer : public frame_producer
{				
	safe_ptr<frame_factory>				factory_;
	safe_ptr<basic_frame>				last_frame_;
	safe_ptr<frame_producer>			current_;
	bool								loop_;

	std::list<safe_ptr<frame_producer>> producers_;

	playlist_producer(const safe_ptr<frame_factory>& factory, bool loop) 
		: factory_(factory)
		, last_frame_(basic_frame::empty())
		, current_(frame_producer::empty())
		, loop_(loop)
	{
	}
	
	virtual safe_ptr<basic_frame> receive(int hints)
	{
		if(current_ == frame_producer::empty() && !producers_.empty())
		{
			current_ = producers_.front();
			producers_.pop_front();
			if(loop_)
				producers_.push_back(current_);
		}

		auto frame = current_->receive(hints);
		if(frame == basic_frame::eof())
		{
			current_ = frame_producer::empty();
			return receive(hints);
		}

		return last_frame_ = frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return disable_audio(last_frame_);
	}

	virtual std::wstring print() const
	{
		return L"playlist[]";
	}	

	virtual int64_t nb_frames() const 
	{
		return std::numeric_limits<int>::max();
	}

	virtual std::wstring param(const std::wstring& param)
	{
		const static std::wstring push_front_str	= L"PUSH_FRONT ";
		const static std::wstring push_back_str		= L"PUSH_BACK ";

		auto pos = param.find(push_front_str);

		if(pos  != std::wstring::npos)
			push_front(param.substr(pos+push_front_str.size()));
				
		pos = param.find(push_back_str);
		
		if(pos  != std::wstring::npos)
			push_back(param.substr(pos+push_back_str.size()));

		return L"";
	}
	
	void push_back(const std::wstring& param)
	{
		producers_.push_back(create_producer(factory_, param)); 
	}

	void push_front(const std::wstring& param)
	{
		producers_.push_front(create_producer(factory_, param)); 
	}
};

safe_ptr<frame_producer> create_playlist_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(boost::range::find(params, L"PLAYLIST") == params.end())
		return core::frame_producer::empty();

	bool loop = boost::range::find(params, L"LOOP") != params.end();

	return make_safe<playlist_producer>(frame_factory, loop);
}

}}

