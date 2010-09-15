#include "../../StdAfx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#endif

#include "cg_producer.h"
#include "flash_producer.h"

#include "../../renderer/render_device.h"
#include "../../frame/frame_format.h"
#include "../../frame/frame.h"
#include "../../Server.h"

#include <boost/filesystem.hpp>
#include <boost/assign.hpp>
#include <tbb/concurrent_unordered_map.h>
		
namespace caspar{ namespace flash{

struct flash_cg_proxy18
{
	std::wstring remove(int layer) 
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		return flashParam.str();
	}

	std::wstring play(int layer) 
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		return flashParam.str();
	}

	std::wstring stop(int layer, unsigned int mix_out_duration)
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><number>") << mix_out_duration << TEXT("</number></arguments></invoke>");
		return flashParam.str();
	}

	std::wstring next(int layer)
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		return flashParam.str();
	}

	std::wstring update(int layer, const std::wstring& data) 
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		return flashParam.str();
	}

	std::wstring invoke(int layer, const std::wstring& label)
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string>") << label << TEXT("</string></arguments></invoke>");
		return flashParam.str();
	}

	std::wstring add(int layer, const std::wstring& template_name, bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		std::wstringstream flashParam;
		flashParam << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << template_name << TEXT("</string>") << (play_on_load?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		return flashParam.str();
	}
};

struct cg_producer::implementation : boost::noncopyable
{
public:

	implementation(const frame_format_desc& fmtDesc) : format_desc_(fmtDesc)
	{
		if(boost::filesystem::exists(server::template_folder()+TEXT("cg.fth.18")))
		{
			flash_producer_ = std::make_shared<flash_producer>(server::template_folder()+TEXT("cg.fth.18"), fmtDesc);
			proxy_.reset(new flash_cg_proxy18());
			CASPAR_LOG(info) << L"Running version 1.8 template graphics.";
		}
		else 
			CASPAR_LOG(info) << L"No templatehost found. Template graphics will be disabled";		
	}

	void clear()
	{
		flash_producer_.reset();
	}

	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking add-command";
		flash_producer_->param(proxy_->add(layer, template_name, play_on_load, startFromLabel, data));
	}

	void remove(int layer)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking remove-command";
		flash_producer_->param(proxy_->remove(layer));
	}

	void play(int layer)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking play-command";
		flash_producer_->param(proxy_->play(layer));
	}

	void stop(int layer, unsigned int mix_out_duration)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking stop-command";
		flash_producer_->param(proxy_->stop(layer, mix_out_duration));
	}

	void next(int layer)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking next-command";
		flash_producer_->param(proxy_->next(layer));
	}

	void update(int layer, const std::wstring& data)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking update-command";
		flash_producer_->param(proxy_->update(layer, data));
	}

	void invoke(int layer, const std::wstring& label)
	{
		if(flash_producer_ == nullptr)
			return;
		CASPAR_LOG(info) << "Invoking invoke-command";
		flash_producer_->param(proxy_->invoke(layer, label));
	}

	frame_ptr get_frame()
	{
		return flash_producer_ ? flash_producer_->get_frame() : nullptr;
	}

	frame_format_desc format_desc_;
	flash_producer_ptr flash_producer_;
	std::unique_ptr<flash_cg_proxy18> proxy_;
};
	

// This is somewhat a hack... needs redesign
cg_producer_ptr get_default_cg_producer(const renderer::render_device_ptr& render_device, unsigned int exLayer)
{
	if(!render_device)
		BOOST_THROW_EXCEPTION(null_argument() << msg_info("render_device"));
	
	auto pProducer = std::dynamic_pointer_cast<cg_producer>(render_device->active(exLayer));
	if(!pProducer)	
	{
		pProducer = std::make_shared<cg_producer>(render_device->frame_format_desc());		
		render_device->load(exLayer, pProducer, renderer::load_option::auto_play); 
	}
	
	return pProducer;
}

cg_producer::cg_producer(const frame_format_desc& fmtDesc) : impl_(new implementation(fmtDesc)){}
frame_ptr cg_producer::get_frame(){return impl_->get_frame();}
void cg_producer::clear(){impl_->clear();}
void cg_producer::add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data){impl_->add(layer, template_name, play_on_load, startFromLabel, data);}
void cg_producer::remove(int layer){impl_->remove(layer);}
void cg_producer::play(int layer){impl_->play(layer);}
void cg_producer::stop(int layer, unsigned int mix_out_duration){impl_->stop(layer, mix_out_duration);}
void cg_producer::next(int layer){impl_->next(layer);}
void cg_producer::update(int layer, const std::wstring& data){impl_->update(layer, data);}
void cg_producer::invoke(int layer, const std::wstring& label){impl_->invoke(layer, label);}
const frame_format_desc& cg_producer::get_frame_format_desc() const { return impl_->format_desc_; }
}}