#include "../../StdAfx.h"

#include "command.h"
#include "exception.h"
#include "list_media.h"

#include "../../producer/media.h"

#include "../../renderer/render_device.h"
#include "../../frame/format.h"

#include "../../server.h"

#include <boost/regex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/range.hpp>
#include <boost/date_time.hpp>

namespace caspar { namespace controller { namespace amcp {
	
std::function<std::wstring()> list_media_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CLS");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	return [=]() -> std::wstring
	{
		CASPAR_LOG(info) << L"Executed [amcp_list_media]";

		return list_media();	
	};
}

std::function<std::wstring()> list_template_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^TLS");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	return [=]() -> std::wstring
	{
		CASPAR_LOG(info) << L"Executed [amcp_list_template]";
		return list_templates();	
	};
}

std::function<std::wstring()> media_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^CINF");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	BOOST_THROW_EXCEPTION(not_implemented());
}

std::function<std::wstring()> template_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^TINF");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	BOOST_THROW_EXCEPTION(not_implemented());
}

std::function<std::wstring()> version_info_command::parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels)
{
	static boost::wregex expr(L"^VERSION");

	boost::wsmatch what;
	if(!boost::regex_match(message, what, expr))
		return nullptr;
	
	return [=]() -> std::wstring
	{
		CASPAR_LOG(info) << L"Executed [amcp_version_info]";
		return std::wstring(TEXT(CASPAR_VERSION_STR)) + TEXT("\r\n");	
	};	
}

}}}
