#include "../../StdAfx.h"

#include "cg_producer.h"

#include "flash_producer.h"

#include "../../Server.h"

#include <boost/filesystem.hpp>
		
namespace caspar { namespace core { namespace flash {
	
struct cg_producer::implementation : boost::noncopyable
{
public:
	implementation() : flash_producer_(flash_producer(server::template_folder()+TEXT("cg.fth.18"))){}

	void clear()
	{
		flash_producer_ = flash_producer(server::template_folder()+TEXT("cg.fth.18"));
	}

	void add(int layer, const std::wstring& filename,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		CASPAR_LOG(info) << "Invoking add-command";
		
		std::wstringstream param;

		param << TEXT("<invoke name=\"Add\" returntype=\"xml\"><arguments><number>") << layer << TEXT("</number><string>") << filename << TEXT("</string>") << (play_on_load?TEXT("<true/>"):TEXT("<false/>")) << TEXT("<string>") << label << TEXT("</string><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	void remove(int layer)
	{
		CASPAR_LOG(info) << "Invoking remove-command";
		
		std::wstringstream param;
		param << TEXT("<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	void play(int layer)
	{
		CASPAR_LOG(info) << "Invoking play-command";
		
		std::wstringstream param;
		param << TEXT("<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	void stop(int layer, unsigned int mix_out_duration)
	{
		CASPAR_LOG(info) << "Invoking stop-command";
		
		std::wstringstream param;
		param << TEXT("<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><number>") << mix_out_duration << TEXT("</number></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	void next(int layer)
	{
		CASPAR_LOG(info) << "Invoking next-command";
		
		std::wstringstream param;
		param << TEXT("<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	void update(int layer, const std::wstring& data)
	{
		CASPAR_LOG(info) << "Invoking update-command";
		
		std::wstringstream param;
		param << TEXT("<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string><![CDATA[ ") << data << TEXT(" ]]></string></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	void invoke(int layer, const std::wstring& label)
	{
		CASPAR_LOG(info) << "Invoking invoke-command";
		
		std::wstringstream param;
		param << TEXT("<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>") << layer << TEXT("</number></property></array><string>") << label << TEXT("</string></arguments></invoke>");
		
		flash_producer_->param(param.str());
	}

	safe_ptr<draw_frame> receive()
	{
		return flash_producer_->receive();
	}
		
	void initialize(const safe_ptr<frame_processor_device>& frame_processor)
	{
		frame_processor_ = frame_processor;
		flash_producer_->initialize(frame_processor);
	}

	std::wstring print() const
	{
		return L"cg[" + flash_producer_->print() + L"]";
	}

	safe_ptr<flash_producer> flash_producer_;
	std::shared_ptr<frame_processor_device> frame_processor_;
};
	
safe_ptr<cg_producer> get_default_cg_producer(const safe_ptr<channel>& channel, int render_layer)
{	
	try
	{
		return dynamic_pointer_cast<cg_producer>(channel->foreground(render_layer).get());
	}
	catch(std::bad_cast&)
	{
		auto producer = make_safe<cg_producer>();		
		channel->load(render_layer, producer, true); 
		return producer;
	}
}

cg_producer::cg_producer() : impl_(new implementation()){}
cg_producer::cg_producer(cg_producer&& other) : impl_(std::move(other.impl_)){}
safe_ptr<draw_frame> cg_producer::receive(){return impl_->receive();}
void cg_producer::clear(){impl_->clear();}
void cg_producer::add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& startFromLabel, const std::wstring& data){impl_->add(layer, template_name, play_on_load, startFromLabel, data);}
void cg_producer::remove(int layer){impl_->remove(layer);}
void cg_producer::play(int layer){impl_->play(layer);}
void cg_producer::stop(int layer, unsigned int mix_out_duration){impl_->stop(layer, mix_out_duration);}
void cg_producer::next(int layer){impl_->next(layer);}
void cg_producer::update(int layer, const std::wstring& data){impl_->update(layer, data);}
void cg_producer::invoke(int layer, const std::wstring& label){impl_->invoke(layer, label);}
void cg_producer::initialize(const safe_ptr<frame_processor_device>& frame_processor){impl_->initialize(frame_processor);}
std::wstring cg_producer::print() const{return impl_->print();}
}}}