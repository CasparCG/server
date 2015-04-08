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

#include "StdAfx.h"

#include "flash.h"
#include "util/swf.h"

#include "producer/flash_producer.h"

#include <common/env.h>
#include <common/os/windows/windows.h>

#include <core/producer/media_info/media_info.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/producer/cg_proxy.h>
#include <core/system_info_provider.h>
#include <core/frame/frame_factory.h>
#include <core/video_format.h>

#include <boost/property_tree/ptree.hpp>
#include <boost/noncopyable.hpp>
#include <boost/filesystem.hpp>

#include <string>
#include <future>

namespace caspar { namespace flash {

std::wstring version();
std::wstring cg_version();

std::wstring get_absolute(const std::wstring& filename)
{
	return (boost::filesystem::path(env::template_folder()) / filename).wstring();
}

class flash_cg_proxy : public core::cg_proxy, boost::noncopyable
{
	spl::shared_ptr<core::frame_producer>	flash_producer_;
	std::wstring							base_folder_;
public:
	explicit flash_cg_proxy(const spl::shared_ptr<core::frame_producer>& producer, std::wstring base_folder = env::template_folder())
		: flash_producer_(producer)
		, base_folder_(base_folder)
	{
	}

	//cg_proxy

	void add(int layer, const std::wstring& template_name, bool play_on_load, const std::wstring& label, const std::wstring& data) override
	{
		auto filename = template_name;

		if (filename.size() > 0 && filename[0] == L'/')
			filename = filename.substr(1, filename.size() - 1);

		filename = (boost::filesystem::path(base_folder_) / filename).wstring();
		filename = find_template(filename);

		auto str = (boost::wformat(L"<invoke name=\"Add\" returntype=\"xml\"><arguments><number>%1%</number><string>%2%</string>%3%<string>%4%</string><string><![CDATA[%5%]]></string></arguments></invoke>") % layer % filename % (play_on_load ? L"<true/>" : L"<false/>") % label % data).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking add-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		flash_producer_->call(std::move(params));
	}

	void remove(int layer) override
	{
		auto str = (boost::wformat(L"<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking remove-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		flash_producer_->call(std::move(params));
	}

	void play(int layer) override
	{
		auto str = (boost::wformat(L"<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking play-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		flash_producer_->call(std::move(params));
	}

	void stop(int layer, unsigned int) override
	{
		auto str = (boost::wformat(L"<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><number>0</number></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking stop-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		flash_producer_->call(std::move(params));
	}

	void next(int layer) override
	{
		auto str = (boost::wformat(L"<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking next-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		flash_producer_->call(std::move(params));
	}

	void update(int layer, const std::wstring& data) override
	{
		auto str = (boost::wformat(L"<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string><![CDATA[%2%]]></string></arguments></invoke>") % layer % data).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking update-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		flash_producer_->call(std::move(params));
	}

	std::wstring invoke(int layer, const std::wstring& label) override
	{
		auto str = (boost::wformat(L"<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array><string>%2%</string></arguments></invoke>") % layer % label).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking invoke-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		// TODO: because of std::async deferred timed waiting does not work so for now we have to block
		return flash_producer_->call(std::move(params)).get();
	}

	std::wstring description(int layer) override
	{
		auto str = (boost::wformat(L"<invoke name=\"GetDescription\" returntype=\"xml\"><arguments><array><property id=\"0\"><number>%1%</number></property></array></arguments></invoke>") % layer).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking description-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		// TODO: because of std::async deferred timed waiting does not work so for now we have to block
		return flash_producer_->call(std::move(params)).get();
	}

	std::wstring template_host_info() override
	{
		auto str = (boost::wformat(L"<invoke name=\"GetInfo\" returntype=\"xml\"><arguments></arguments></invoke>")).str();
		CASPAR_LOG(info) << flash_producer_->print() << " Invoking info-command: " << str;
		std::vector<std::wstring> params;
		params.push_back(std::move(str));
		// TODO: because of std::async deferred timed waiting does not work so for now we have to block
		return flash_producer_->call(std::move(params)).get();
	}
};

spl::shared_ptr<core::frame_producer> create_ct_producer(
		const spl::shared_ptr<core::frame_factory> frame_factory,
		const core::video_format_desc& format_desc,
		const std::vector<std::wstring>& params)
{
	auto flash_producer = flash::create_producer(frame_factory, format_desc, {});
	auto producer = flash_producer;
	flash_cg_proxy(producer, env::media_folder()).add(0, params[0], true, L"", L"");

	return producer;
}

void init(
		const spl::shared_ptr<core::media_info_repository>& media_info_repo,
		const spl::shared_ptr<core::system_info_provider_repository>& info_provider_repo,
		const spl::shared_ptr<core::cg_producer_registry>& cg_registry)
{
	core::register_producer_factory(create_ct_producer);
	core::register_producer_factory(create_swf_producer);
	media_info_repo->register_extractor([](const std::wstring& file, const std::wstring& extension, core::media_info& info)
	{
		if (extension != L".CT" && extension != L".SWF")
			return false;

		info.clip_type = L"MOVIE";

		return true;
	});
	info_provider_repo->register_system_info_provider([](boost::property_tree::wptree& info)
	{
		info.add(L"system.flash", version());
	});
	info_provider_repo->register_version_provider(L"FLASH", &version);
	info_provider_repo->register_version_provider(L"TEMPLATEHOST", &cg_version);
	cg_registry->register_cg_producer(
			L"flash",
			{ L".ft", L".ct" },
			[](const std::wstring& filename)
			{
				return read_template_meta_info(get_absolute(filename) + L".ft");
			},
			[](const spl::shared_ptr<core::frame_producer>& producer)
			{
				return spl::make_shared<flash_cg_proxy>(producer);
			},
			[](const spl::shared_ptr<core::frame_factory>& ff, const core::video_format_desc& f, const std::wstring&)
			{
				return flash::create_producer(ff, f, { });
			},
			true
		);
}

std::wstring cg_version()
{
	return L"Unknown";
}

std::wstring version()
{		
	std::wstring version = L"Not found";
#ifdef WIN32 
	HKEY   hkey;
 
	DWORD dwType, dwSize;
	if(RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Macromedia\\FlashPlayerActiveX"), 0, KEY_QUERY_VALUE, &hkey) == ERROR_SUCCESS)
	{
		wchar_t ver_str[1024];

		dwType = REG_SZ;
		dwSize = sizeof(ver_str);
		RegQueryValueEx(hkey, TEXT("Version"), NULL, &dwType, (PBYTE)&ver_str, &dwSize);
 
		version = ver_str;

		RegCloseKey(hkey);
	}
#endif
	return version;
}

}}
