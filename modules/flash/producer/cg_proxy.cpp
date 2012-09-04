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

#include "cg_proxy.h"

#include "flash_producer.h"

#include <common/env.h>

#include <core/mixer/mixer.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/ptree.hpp>
		
namespace caspar { namespace flash {
	
struct cg_proxy::impl : boost::noncopyable
{
	spl::shared_ptr<core::frame_producer> flash_producer_;
public:
	impl(const spl::shared_ptr<core::frame_producer>& frame_producer) 
		: flash_producer_(frame_producer)
	{}
	
	boost::unique_future<std::wstring> add(int layer, std::wstring filename,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		if(filename.size() > 0 && filename[0] == L'/')
			filename = filename.substr(1, filename.size()-1);

		if(boost::filesystem::wpath(filename).extension() == L"")
			filename += L".ft";
		
		auto str = (boost::wformat(L"<invoke name=\"Add\" returntype=\"xml\"><arguments><number>%1%</number><string>%2%</string>%3%<string>%4%</string><string><![CDATA[%5%]]></string></arguments></invoke>") % layer % filename % (play_on_load?L"<true/>":L"<false/>") % label % data).str();

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
	
cg_proxy create_cg_proxy(const spl::shared_ptr<core::video_channel>& video_channel, int render_layer)
{	
	auto flash_producer = video_channel->stage().foreground(render_layer).get();

	try
	{
		if(flash_producer->name() != L"flash")
		{
			flash_producer = flash::create_producer(video_channel->frame_factory(), video_channel->video_format_desc(), boost::assign::list_of<std::wstring>());	
			video_channel->stage().load(render_layer, flash_producer); 
			video_channel->stage().play(render_layer);
		}
	}
	catch(...)
	{
		CASPAR_LOG_CURRENT_EXCEPTION();
		throw;
	}

	return cg_proxy(std::move(flash_producer));
}

spl::shared_ptr<core::frame_producer> create_cg_producer_and_autoplay_file(
		const spl::shared_ptr<core::frame_factory> frame_factory, 
		const core::video_format_desc& format_desc, 
		const std::vector<std::wstring>& params,
		const std::wstring& filename) 
{
	if(!boost::filesystem::exists(filename))
		return core::frame_producer::empty();
		
	boost::filesystem::path path(filename);
	path = boost::filesystem3::complete(path);
	auto filename2 = path.wstring();

	auto flash_producer = flash::create_producer(frame_factory, format_desc, boost::assign::list_of<std::wstring>());	
	auto producer = flash_producer;
	cg_proxy(producer).add(0, filename2, 1);

	return producer;
}

spl::shared_ptr<core::frame_producer> create_ct_producer(const spl::shared_ptr<core::frame_factory> frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params) 
{
	return create_cg_producer_and_autoplay_file(
		frame_factory, format_desc, params, env::media_folder() + L"\\" + params[0] + L".ct");
}

spl::shared_ptr<core::frame_producer> create_swf_producer(const spl::shared_ptr<core::frame_factory> frame_factory, const core::video_format_desc& format_desc, const std::vector<std::wstring>& params) 
{
	return create_cg_producer_and_autoplay_file(
		frame_factory, format_desc, params, env::media_folder() + L"\\" + params[0] + L".swf");
}

cg_proxy::cg_proxy(const spl::shared_ptr<core::frame_producer>& frame_producer) : impl_(new impl(frame_producer)){}
cg_proxy::cg_proxy(cg_proxy&& other) : impl_(std::move(other.impl_)){}
void cg_proxy::add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data){impl_->add(layer, template_name, play_on_load, startFromLabel, data);}
void cg_proxy::remove(int layer){impl_->remove(layer);}
void cg_proxy::play(int layer){impl_->play(layer);}
void cg_proxy::stop(int layer, unsigned int mix_out_duration){impl_->stop(layer, mix_out_duration);}
void cg_proxy::next(int layer){impl_->next(layer);}
void cg_proxy::update(int layer, const std::wstring& data){impl_->update(layer, data);}
std::wstring cg_proxy::invoke(int layer, const std::wstring& label){return impl_->timed_invoke(layer, label);}
std::wstring cg_proxy::description(int layer){return impl_->timed_description(layer);}
std::wstring cg_proxy::template_host_info(){return impl_->timed_template_host_info();}

}}