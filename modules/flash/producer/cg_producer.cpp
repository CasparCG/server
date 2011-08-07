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
#include "../StdAfx.h"

#include "cg_producer.h"

#include "flash_producer.h"

#include <common/env.h>

#include <core/mixer/mixer.h>

#include <boost/filesystem.hpp>
#include <boost/format.hpp>
		
namespace caspar {
	
struct cg_producer::implementation : boost::noncopyable
{
	safe_ptr<core::frame_producer> flash_producer_;
public:
	implementation(const safe_ptr<core::frame_producer>& frame_producer) 
		: flash_producer_(frame_producer)
	{}
	
	void add(int layer, std::wstring filename,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		if(filename.size() > 0 && filename[0] == L'/')
			filename = filename.substr(1, filename.size()-1);

		auto str = (boost::wformat(L"<invoke name=\"Add\" returntype=\"xml\"><arguments><number>%1%</number><string>%2%</string>%3%<string>%4%</string><string><![CDATA[%5%]]></string></arguments></invoke>") % layer % filename % (play_on_load?TEXT("<true/>"):TEXT("<false/>")) % label % data).str();

		CASPAR_LOG(info) << flash_producer_->print() << " Invoking add-command:" << str;
		flash_producer_->param(str);
	}

	void remove(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking remove-command:" << str;
		flash_producer_->param(str);
	}

	void play(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking play-command: " << str;
		flash_producer_->param(str);
	}

	void stop(int layer, unsigned int)
	{
		auto str = (boost::wformat(L"<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><number>0</number></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking stop-command:" << str;
		flash_producer_->param(str);
	}

	void next(int layer)
	{
		auto str = (boost::wformat(L"<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking next-command:" << str;
		flash_producer_->param(str);
	}

	void update(int layer, const std::wstring& data)
	{
		auto str = (boost::wformat(L"<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string><![CDATA[%2%]]></string></arguments></invoke>") % layer % data).str();
		CASPAR_LOG(info) << flash_producer_->print() <<" Invoking update-command:" << str;
		flash_producer_->param(str);
	}

	void invoke(int layer, const std::wstring& label)
	{
		auto str = (boost::wformat(L"<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string>%2%</string></arguments></invoke>") % layer % label).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking invoke-command: " << str;
		flash_producer_->param(str);
	}

	virtual safe_ptr<core::basic_frame> receive(int hints)
	{
		return flash_producer_->receive(hints);
	}

	virtual safe_ptr<core::basic_frame> last_frame() const
	{
		return flash_producer_->last_frame();
	}			
			
	std::wstring print() const
	{
		return flash_producer_->print();
	}
};
	
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<core::video_channel>& video_channel, int render_layer)
{	
	auto flash_producer = video_channel->stage()->foreground(render_layer).get();

	if(flash_producer->print().find(L"flash[") == std::string::npos) // UGLY hack
	{
		flash_producer = create_flash_producer(video_channel->mixer(), boost::assign::list_of(env::template_host()));	
		video_channel->stage()->load(render_layer, flash_producer); 
		video_channel->stage()->play(render_layer);
	}

	return make_safe<cg_producer>(flash_producer);
}

safe_ptr<core::frame_producer> create_ct_producer(const safe_ptr<core::frame_factory> frame_factory, const std::vector<std::wstring>& params) 
{
	std::wstring filename = env::media_folder() + L"\\" + params[0] + L".ct";
	if(!boost::filesystem::exists(filename))
		return core::frame_producer::empty();
	
	auto flash_producer = create_flash_producer(frame_factory, boost::assign::list_of(env::template_host()));	
	auto producer = make_safe<cg_producer>(flash_producer);
	producer->add(0, filename, 1);

	return producer;
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
void cg_producer::invoke(int layer, const std::wstring& label){impl_->invoke(layer, label);}
std::wstring cg_producer::print() const{return impl_->print();}

}