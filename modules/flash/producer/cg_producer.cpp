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

#include "../StdAfx.h"

#include "cg_producer.h"

#include "flash_producer.h"

#include <common/env.h>

#include <core/mixer/mixer.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
		
namespace caspar { namespace flash {
	
struct cg_producer::implementation : boost::noncopyable
{
	safe_ptr<core::frame_producer> flash_producer_;
public:
	implementation(const safe_ptr<core::frame_producer>& frame_producer) 
		: flash_producer_(frame_producer)
	{}
	
	boost::unique_future<std::wstring> add(int layer, std::wstring filename,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		if(filename.size() > 0 && filename[0] == L'/')
			filename = filename.substr(1, filename.size()-1);

		if(boost::filesystem::wpath(filename).extension() == L"")
			filename += L".ft";
		
		auto str = (boost::wformat(L"<invoke name=\"Add\" returntype=\"xml\"><arguments><number>%1%</number><string>%2%</string>%3%<string>%4%</string><string><![CDATA[%5%]]></string></arguments></invoke>") % layer % filename % (play_on_load?TEXT("<true/>"):TEXT("<false/>")) % label % data).str();

		CASPAR_LOG(info) << flash_producer_->print() << " Invoking add-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> remove(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking remove-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> play(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking play-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> stop(int layer, unsigned int)
	{
		auto str = (boost::wformat(L"<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><number>0</number></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking stop-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> next(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking next-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> update(int layer, const std::wstring& data)
	{
		auto str = (boost::wformat(L"<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string><![CDATA[%2%]]></string></arguments></invoke>") % layer % data).str();
		CASPAR_LOG(info) << flash_producer_->print() <<" Invoking update-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> invoke(int layer, const std::wstring& label)
	{
		auto str = (boost::wformat(L"<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string>%2%</string></arguments></invoke>") % layer % label).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking invoke-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> description(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"GetDescription\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking description-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> template_host_info()
	{
		auto str = (boost::wformat(L"<invoke name=\"GetInfo\" returntype=\"xml\"><arguments></arguments></invoke>")).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking info-command: " << str;
		return flash_producer_->call(str);
	}

	boost::unique_future<std::wstring> call(const std::wstring& str)
	{		
		static const boost::wregex add_exp			(L"ADD (?<LAYER>\\d+) (?<FILENAME>[^\\s]+) (?<PLAY_ON_LOAD>\\d)( (?<DATA>.*))?");
		static const boost::wregex remove_exp		(L"REMOVE (?<LAYER>\\d+)");
		static const boost::wregex play_exp			(L"PLAY (?<LAYER>\\d+)");
		static const boost::wregex stop_exp			(L"STOP (?<LAYER>\\d+)");
		static const boost::wregex next_exp			(L"NEXT (?<LAYER>\\d+)");
		static const boost::wregex update_exp		(L"UPDATE (?<LAYER>\\d+) (?<DATA>.+)");
		static const boost::wregex invoke_exp		(L"INVOKE (?<LAYER>\\d+) (?<LABEL>.+)");
		static const boost::wregex description_exp	(L"INFO (?<LAYER>\\d+)");
		static const boost::wregex info_exp			(L"INFO");
		
		boost::wsmatch what;
		if(boost::regex_match(str, what, add_exp))
			return add(boost::lexical_cast<int>(what["LAYER"].str()), flash::find_template(env::template_folder() + what["FILENAME"].str()), boost::lexical_cast<bool>(what["PLAY_ON_LOAD"].str()), L"", what["DATA"].str()); 
		else if(boost::regex_match(str, what, remove_exp))
			return remove(boost::lexical_cast<int>(what["LAYER"].str())); 
		else if(boost::regex_match(str, what, stop_exp))
			return stop(boost::lexical_cast<int>(what["LAYER"].str()), 0); 
		else if(boost::regex_match(str, what, next_exp))
			return next(boost::lexical_cast<int>(what["LAYER"].str())); 
		else if(boost::regex_match(str, what, update_exp))
			return update(boost::lexical_cast<int>(what["LAYER"].str()), what["DATA"].str()); 
		else if(boost::regex_match(str, what, next_exp))
			return invoke(boost::lexical_cast<int>(what["LAYER"].str()), what["LABEL"].str()); 
		else if(boost::regex_match(str, what, description_exp))
			return description(boost::lexical_cast<int>(what["LAYER"].str())); 
		else if(boost::regex_match(str, what, invoke_exp))
			return template_host_info(); 

		return flash_producer_->call(str);
	}

	safe_ptr<core::basic_frame> receive(int hints)
	{
		return flash_producer_->receive(hints);
	}

	safe_ptr<core::basic_frame> last_frame() const
	{
		return flash_producer_->last_frame();
	}		
			
	std::wstring print() const
	{
		return flash_producer_->print();
	}

	boost::property_tree::wptree info() const
	{
		boost::property_tree::wptree info;
		info.add(L"type", L"cg-producer");
		return info;
	}

	std::wstring timed_invoke(int layer, const std::wstring& label)
	{
		auto result = invoke(layer, label);
		if(result.timed_wait(boost::posix_time::seconds(2)))
			return result.get();
		return L"";
	}
	std::wstring timed_description(int layer)
	{
		auto result = description(layer);
		if(result.timed_wait(boost::posix_time::seconds(2)))
			return result.get();
		return L"";
	}
	std::wstring timed_template_host_info()
	{
		auto result = template_host_info();
		if(result.timed_wait(boost::posix_time::seconds(2)))
			return result.get();
		return L"";
	}
};
	
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<core::video_channel>& video_channel, int render_layer)
{	
	auto flash_producer = video_channel->stage()->foreground(render_layer).get();

	try
	{
		if(flash_producer->print().find(L"flash[") == std::string::npos) // UGLY hack
		{
			flash_producer = flash::create_producer(video_channel->mixer(), boost::assign::list_of<std::wstring>());	
			video_channel->stage()->load(render_layer, flash_producer); 
			video_channel->stage()->play(render_layer);
		}
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		throw;
	}

	return make_safe<cg_producer>(flash_producer);
}

safe_ptr<core::frame_producer> create_cg_producer_and_autoplay_file(
		const safe_ptr<core::frame_factory>& frame_factory, 
		const std::vector<std::wstring>& params,
		const std::wstring& filename) 
{
	if(!boost::filesystem::exists(filename))
		return core::frame_producer::empty();
		
	boost::filesystem2::wpath path(filename);
	path = boost::filesystem2::complete(path);
	auto filename2 = path.file_string();

	auto flash_producer = flash::create_producer(frame_factory, boost::assign::list_of<std::wstring>());	
	auto producer = make_safe<cg_producer>(flash_producer);
	producer->add(0, filename2, 1);

	return producer;
}

safe_ptr<core::frame_producer> create_ct_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params) 
{
	return create_cg_producer_and_autoplay_file(frame_factory, params, env::media_folder() + L"\\" + params[0] + L".ct");
}

safe_ptr<core::frame_producer> create_cg_producer(const safe_ptr<core::frame_factory>& frame_factory, const std::vector<std::wstring>& params) 
{
	if(params.empty() || params.at(0) != L"[CG]")
		return core::frame_producer::empty();

	return make_safe<cg_producer>(flash::create_producer(frame_factory, boost::assign::list_of<std::wstring>()));	
}

cg_producer::cg_producer(const safe_ptr<core::frame_producer>& frame_producer) : impl_(new implementation(frame_producer)){}
cg_producer::cg_producer(cg_producer&& other) : impl_(std::move(other.impl_)){}
safe_ptr<core::basic_frame> cg_producer::receive(int hints){return impl_->receive(hints);}
safe_ptr<core::basic_frame> cg_producer::last_frame() const{return impl_->last_frame();}
void cg_producer::add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data){impl_->add(layer, template_name, play_on_load, startFromLabel, data);}
void cg_producer::remove(int layer){impl_->remove(layer);}
void cg_producer::play(int layer){impl_->play(layer);}
void cg_producer::stop(int layer, unsigned int mix_out_duration){impl_->stop(layer, mix_out_duration);}
void cg_producer::next(int layer){impl_->next(layer);}
void cg_producer::update(int layer, const std::wstring& data){impl_->update(layer, data);}
std::wstring cg_producer::print() const{return impl_->print();}
boost::unique_future<std::wstring> cg_producer::call(const std::wstring& str){return impl_->call(str);}
std::wstring cg_producer::invoke(int layer, const std::wstring& label){return impl_->timed_invoke(layer, label);}
std::wstring cg_producer::description(int layer){return impl_->timed_description(layer);}
std::wstring cg_producer::template_host_info(){return impl_->timed_template_host_info();}
boost::property_tree::wptree cg_producer::info() const{return impl_->info();}

}}