#pragma once

#include "renderer/renderer_fwd.h"

namespace caspar { namespace core { 
	
struct invalid_configuration : virtual boost::exception, virtual std::exception {};

class server : boost::noncopyable
{
public:
	server();

	static const std::wstring& media_folder();
	static const std::wstring& log_folder();
	static const std::wstring& template_folder();		
	static const std::wstring& data_folder();	

	const std::vector<renderer::render_device_ptr>& get_channels() const;
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}}