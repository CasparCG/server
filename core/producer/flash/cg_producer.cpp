#include "../../StdAfx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4714) // marked as __forceinline not inlined
#endif

#include "cg_producer.h"

#include "flash_producer.h"

#include "../../processor/draw_frame.h"
#include "../../Server.h"

#include <boost/filesystem.hpp>
#include <boost/assign.hpp>
		
namespace caspar { namespace core { namespace flash {
	
struct template_version
{
	enum type
	{
		_17,
		_18,
		invalid,
		count
	};
};

struct cg_producer::implementation : boost::noncopyable
{
public:

	implementation() : ver_(template_version::invalid), flash_producer_(create_flash()){}

	safe_ptr<flash_producer> create_flash()
	{		
		if(boost::filesystem::exists(server::template_folder()+TEXT("cg.fth.18")))
		{
			CASPAR_LOG(info) << L"Running version 1.8 template graphics.";
			ver_ = template_version::_18;
			return safe_ptr<flash_producer>(flash_producer(server::template_folder()+TEXT("cg.fth.18")));
		}
		else if(boost::filesystem::exists(server::template_folder()+TEXT("cg.fth.17")))
		{
			CASPAR_LOG(info) << L"Running version 1.7 template graphics.";
			ver_ = template_version::_17;
			return safe_ptr<flash_producer>(flash_producer(server::template_folder()+TEXT("cg.fth.17")));
		}
		else 
			BOOST_THROW_EXCEPTION(file_not_found() << msg_info("No templatehost found."));	
	}

	void clear()
	{
		flash_producer_ = create_flash();
	}

	void add(int layer, const std::wstring& template_name,  bool play_on_load, const std::wstring& label, const std::wstring& data)
	{
		CASPAR_LOG(info) << "Invoking add-command";
		
		std::wstringstream param;

		std::wstring filename = template_name;

		if(ver_ == template_version::_17)
		{
			std::wstring::size_type pos = template_name.find('.');
			filename = (pos != std::wstring::npos) ? template_name.substr(0, pos) : template_name;
		}

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
	template_version::type ver_;
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