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

#include <common/env.h>

#include "cg_producer.h"

#include "flash_producer.h"

#include <boost/filesystem.hpp>
#include <boost/format.hpp>

#include <boost/assign.hpp>

using boost::format;
using boost::io::group;
		
namespace caspar {
	
struct cg_producer::implementation : boost::noncopyable
{
	safe_ptr<core::frame_factory> frame_factory_;
	safe_ptr<core::frame_producer> flash_producer_;
public:
	implementation(const safe_ptr<core::frame_factory>& frame_factory) 
		: frame_factory_(frame_factory)
		, flash_producer_(create_flash_producer(frame_factory_, boost::assign::list_of(env::template_host())))
	{}
	
	void add(int layer, const std::wstring& filename,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking add-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Add\" returntype=\"xml\"><arguments><number>%1%</number><string>%2%</string>%3%<string>%4%</string><string><![CDATA[%5%]]></string></arguments></invoke>") % layer % filename % (play_on_load?TEXT("<true/>"):TEXT("<false/>")) % label % data).str());
	}

	void remove(int layer)
	{
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking remove-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void play(int layer)
	{
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking play-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void stop(int layer, unsigned int)
	{
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking stop-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><number>0</number></arguments></invoke>") % layer).str());
	}

	void next(int layer)
	{
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking next-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str());
	}

	void update(int layer, const std::wstring& data)
	{
		CASPAR_LOG(info) << flash_producer_->print() <<" Invoking update-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string><![CDATA[%2%]]></string></arguments></invoke>") % layer % data).str());
	}

	void invoke(int layer, const std::wstring& label)
	{
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking invoke-command";
		flash_producer_->param((boost::wformat(L"<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string>%2%</string></arguments></invoke>") % layer % label).str());
	}

	safe_ptr<core::basic_frame> receive()
	{
		return flash_producer_->receive();
	}
			
	std::wstring print() const
	{
		return flash_producer_->print();
	}
};
	
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<core::channel>& channel, int render_layer)
{	
	try
	{
		return dynamic_pointer_cast<cg_producer>(channel->producer()->foreground(render_layer).get());
	}
	catch(std::bad_cast&)
	{
		safe_ptr<core::frame_factory> factory = channel->mixer();
		auto producer = make_safe<cg_producer>(factory);		
		channel->producer()->load(render_layer, producer, true); 
		channel->producer()->play(render_layer);
		return producer;
	}
}

safe_ptr<core::frame_producer> create_ct_producer(const safe_ptr<core::frame_factory> frame_factory, const std::vector<std::wstring>& params) 
{
	std::wstring filename = env::media_folder() + L"\\" + params[0] + L".ct";
	if(!boost::filesystem::exists(filename))
		return core::frame_producer::empty();

	auto producer = make_safe<cg_producer>(frame_factory);
	producer->add(0, filename, 1);

	return producer;
}

cg_producer::cg_producer(const safe_ptr<core::frame_factory>& frame_factory) : impl_(new implementation(frame_factory)){}
cg_producer::cg_producer(cg_producer&& other) : impl_(std::move(other.impl_)){}
safe_ptr<core::basic_frame> cg_producer::receive(){return impl_->receive();}
void cg_producer::add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data){impl_->add(layer, template_name, play_on_load, startFromLabel, data);}
void cg_producer::remove(int layer){impl_->remove(layer);}
void cg_producer::play(int layer){impl_->play(layer);}
void cg_producer::stop(int layer, unsigned int mix_out_duration){impl_->stop(layer, mix_out_duration);}
void cg_producer::next(int layer){impl_->next(layer);}
void cg_producer::update(int layer, const std::wstring& data){impl_->update(layer, data);}
void cg_producer::invoke(int layer, const std::wstring& label){impl_->invoke(layer, label);}
std::wstring cg_producer::print() const{return impl_->print();}

}