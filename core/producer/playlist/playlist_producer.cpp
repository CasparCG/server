/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "../../stdafx.h"

#include "playlist_producer.h"

#include <core/producer/frame_producer.h>
#include <core/producer/frame/basic_frame.h>

#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>

#include <deque>

namespace caspar { namespace core {	

struct playlist_producer : public frame_producer
{				
	safe_ptr<frame_factory>				factory_;
	safe_ptr<basic_frame>				last_frame_;
	safe_ptr<frame_producer>			current_;
	bool								loop_;

	std::deque<safe_ptr<frame_producer>> producers_;

	playlist_producer(const safe_ptr<frame_factory>& factory, bool loop) 
		: factory_(factory)
		, last_frame_(basic_frame::empty())
		, current_(frame_producer::empty())
		, loop_(loop)
	{
	}

	// frame_producer
	
	virtual safe_ptr<basic_frame> receive(int hints) override
	{
		if(current_ == frame_producer::empty() && !producers_.empty())
			next();

		auto frame = current_->receive(hints);
		if(frame == basic_frame::eof())
		{
			current_ = frame_producer::empty();
			return receive(hints);
		}

		return last_frame_ = frame;
	}

	virtual safe_ptr<core::basic_frame> last_frame() const override
	{
		return disable_audio(last_frame_);
	}

	virtual std::wstring print() const override
	{
		return L"playlist[" + current_->print() + L"]";
	}	

	virtual boost::property_tree::wptree info() const override
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"playlist-producer");
		return info;
	}

	virtual uint32_t nb_frames() const  override
	{
		return std::numeric_limits<uint32_t>::max();
	}
	
	virtual boost::unique_future<std::wstring> call(const std::wstring& param) override
	{
		boost::promise<std::wstring> promise;
		promise.set_value(do_call(param));
		return promise.get_future();
	}	

	// playlist_producer

	std::wstring do_call(const std::wstring& param)
	{		
		static const boost::wregex push_front_exp	(L"PUSH_FRONT (?<PARAM>.+)");		
		static const boost::wregex push_back_exp	(L"(PUSH_BACK|PUSH) (?<PARAM>.+)");
		static const boost::wregex pop_front_exp	(L"POP_FRONT");		
		static const boost::wregex pop_back_exp		(L"(POP_BACK|POP)");
		static const boost::wregex clear_exp		(L"CLEAR");
		static const boost::wregex next_exp			(L"NEXT");
		static const boost::wregex insert_exp		(L"INSERT (?<POS>\\d+) (?<PARAM>.+)");	
		static const boost::wregex remove_exp		(L"REMOVE (?<POS>\\d+) (?<PARAM>.+)");	
		static const boost::wregex list_exp			(L"LIST");			
		static const boost::wregex loop_exp			(L"LOOP\\s*(?<VALUE>\\d?)");
		
		boost::wsmatch what;

		if(boost::regex_match(param, what, push_front_exp))
			return push_front(what["PARAM"].str()); 
		else if(boost::regex_match(param, what, push_back_exp))
			return push_back(what["PARAM"].str()); 
		if(boost::regex_match(param, what, pop_front_exp))
			return pop_front(); 
		else if(boost::regex_match(param, what, pop_back_exp))
			return pop_back(); 
		else if(boost::regex_match(param, what, clear_exp))
			return clear();
		else if(boost::regex_match(param, what, next_exp))
			return next(); 
		else if(boost::regex_match(param, what, insert_exp))
			return insert(boost::lexical_cast<size_t>(what["POS"].str()), what["PARAM"].str());
		else if(boost::regex_match(param, what, remove_exp))
			return erase(boost::lexical_cast<size_t>(what["POS"].str()));
		else if(boost::regex_match(param, what, list_exp))
			return list();
		else if(boost::regex_match(param, what, loop_exp))
		{
			if(!what["VALUE"].str().empty())
				loop_ = boost::lexical_cast<bool>(what["VALUE"].str());
			return boost::lexical_cast<std::wstring>(loop_);
		}

		BOOST_THROW_EXCEPTION(invalid_argument());
	}
	
	std::wstring push_front(const std::wstring& str)
	{
		producers_.push_front(create_producer(factory_, str)); 
		return L"";
	}

	std::wstring  push_back(const std::wstring& str)
	{
		producers_.push_back(create_producer(factory_, str)); 
		return L"";
	}

	std::wstring pop_front()
	{
		producers_.pop_front();
		return L"";
	}

	std::wstring pop_back()
	{
		producers_.pop_back();
		return L"";
	}
	
	std::wstring clear()
	{
		producers_.clear();
		return L"";
	}

	std::wstring next()
	{
		if(!producers_.empty())
		{
			current_ = producers_.front();
			producers_.pop_front();
			//if(loop_)
			//	producers_.push_back(current_);
		}
		return L"";
	}
	
	std::wstring  insert(size_t pos, const std::wstring& str)
	{
		if(pos >= producers_.size())
			BOOST_THROW_EXCEPTION(out_of_range());
		producers_.insert(std::begin(producers_) + pos, create_producer(factory_, str));
		return L"";
	}

	std::wstring  erase(size_t pos)
	{
		if(pos >= producers_.size())
			BOOST_THROW_EXCEPTION(out_of_range());
		producers_.erase(std::begin(producers_) + pos);
		return L"";
	}

	std::wstring list() const
	{
		std::wstring result = L"<playlist>\n";
		BOOST_FOREACH(auto& producer, producers_)		
			result += L"\t<producer>" + producer->print() + L"</producer>\n";
		return result + L"</playlist>";
	}
};

safe_ptr<frame_producer> create_playlist_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params)
{
	if(boost::range::find(params, L"[PLAYLIST]") == params.end())
		return core::frame_producer::empty();

	bool loop = boost::range::find(params, L"LOOP") != params.end();

	return make_safe<playlist_producer>(frame_factory, loop);
}

}}

