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

#include <core/producer/cg_proxy.h>
#include <core/video_format.h>

#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

#include <future>
#include <string>
#include <utility>

namespace caspar { namespace flash {

std::wstring version();
std::wstring cg_version();

std::wstring get_absolute(const std::wstring& base_folder, const std::wstring& filename)
{
    return (boost::filesystem::path(base_folder) / filename).wstring();
}

class flash_cg_proxy : public core::cg_proxy
{
    spl::shared_ptr<core::frame_producer> flash_producer_;
    std::wstring                          base_folder_;

    flash_cg_proxy(const flash_cg_proxy&) = delete;
    flash_cg_proxy& operator=(const flash_cg_proxy&) = delete;

  public:
    explicit flash_cg_proxy(const spl::shared_ptr<core::frame_producer>& producer,
                            std::wstring                                 base_folder = env::template_folder())
        : flash_producer_(producer)
        , base_folder_(std::move(base_folder))
    {
    }

    // cg_proxy

    void add(int                 layer,
             const std::wstring& template_name,
             bool                play_on_load,
             const std::wstring& label,
             const std::wstring& data) override
    {
        auto filename = template_name;

        if (!filename.empty() && filename[0] == L'/')
            filename = filename.substr(1, filename.size() - 1);

        filename = (boost::filesystem::path(base_folder_) / filename).wstring();
        filename = find_template(filename);

        auto str = (boost::wformat(L"<invoke name=\"Add\" "
                                   L"returntype=\"xml\"><arguments><number>%1%</number><string>%2%</"
                                   L"string>%3%<string>%4%</string><string><![CDATA[%5%]]></"
                                   L"string></arguments></invoke>") %
                    layer % filename % (play_on_load ? L"<true/>" : L"<false/>") % label % data)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking add-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        flash_producer_->call(std::move(params)).get();
    }

    void verify_flash_player()
    {
        if (flash_producer_->call({L"?"}).get() == L"0")
            CASPAR_THROW_EXCEPTION(expected_user_error() << msg_info("No flash player running on video layer."));
    }

    void remove(int layer) override
    {
        verify_flash_player();

        auto str = (boost::wformat(L"<invoke name=\"Delete\" returntype=\"xml\"><arguments><array><property "
                                   L"id=\"0\"><number>%1%</number></property></array></arguments></invoke>") %
                    layer)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking remove-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        flash_producer_->call(std::move(params));
    }

    void play(int layer) override
    {
        verify_flash_player();

        auto str = (boost::wformat(L"<invoke name=\"Play\" returntype=\"xml\"><arguments><array><property "
                                   L"id=\"0\"><number>%1%</number></property></array></arguments></invoke>") %
                    layer)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking play-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        flash_producer_->call(std::move(params));
    }

    void stop(int layer) override
    {
        verify_flash_player();

        auto str = (boost::wformat(
                        L"<invoke name=\"Stop\" returntype=\"xml\"><arguments><array><property "
                        L"id=\"0\"><number>%1%</number></property></array><number>0</number></arguments></invoke>") %
                    layer)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking stop-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        flash_producer_->call(std::move(params));
    }

    void next(int layer) override
    {
        verify_flash_player();

        auto str = (boost::wformat(L"<invoke name=\"Next\" returntype=\"xml\"><arguments><array><property "
                                   L"id=\"0\"><number>%1%</number></property></array></arguments></invoke>") %
                    layer)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking next-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        flash_producer_->call(std::move(params));
    }

    void update(int layer, const std::wstring& data) override
    {
        verify_flash_player();

        auto str = (boost::wformat(L"<invoke name=\"SetData\" returntype=\"xml\"><arguments><array><property "
                                   L"id=\"0\"><number>%1%</number></property></array><string><![CDATA[%2%]]></string></"
                                   L"arguments></invoke>") %
                    layer % data)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking update-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        flash_producer_->call(std::move(params));
    }

    std::wstring invoke(int layer, const std::wstring& label) override
    {
        verify_flash_player();

        auto str = (boost::wformat(
                        L"<invoke name=\"Invoke\" returntype=\"xml\"><arguments><array><property "
                        L"id=\"0\"><number>%1%</number></property></array><string>%2%</string></arguments></invoke>") %
                    layer % label)
                       .str();
        CASPAR_LOG(debug) << flash_producer_->print() << " Invoking invoke-command: " << str;
        std::vector<std::wstring> params;
        params.push_back(std::move(str));
        // TODO: because of std::async deferred timed waiting does not work so for now we have to block
        return flash_producer_->call(std::move(params)).get();
    }
};

spl::shared_ptr<core::frame_producer> create_ct_producer(const core::frame_producer_dependencies& dependencies,
                                                         const std::vector<std::wstring>&         params)
{
    if (params.empty() || !boost::filesystem::exists(get_absolute(env::media_folder(), params.at(0)) + L".ct"))
        return core::frame_producer::empty();

    auto flash_producer = flash::create_producer(dependencies, {});
    auto producer       = flash_producer;
    flash_cg_proxy(producer, env::media_folder()).add(0, params.at(0), true, L"", L"");

    return producer;
}

void copy_template_hosts()
{
    try {
        for (auto it = boost::filesystem::directory_iterator(env::initial_folder());
             it != boost::filesystem::directory_iterator();
             ++it) {
            if (it->path().wstring().find(L".fth") != std::wstring::npos) {
                auto from_path = *it;
                auto to_path = boost::filesystem::path(env::template_folder() + L"/" + it->path().filename().wstring());

                if (boost::filesystem::exists(to_path))
                    boost::filesystem::remove(to_path);

                boost::filesystem::copy_file(from_path, to_path);
            }
        }
    } catch (...) {
        CASPAR_LOG_CURRENT_EXCEPTION();
        CASPAR_LOG(error) << L"Failed to copy template-hosts from initial-path to template-path.";
    }
}

void init(core::module_dependencies dependencies)
{
    if (env::properties().get(L"configuration.flash.enabled", false)) {
        CASPAR_LOG(warning) << "Flash is no longer a recommended way of creating dynamic templates. Adobe have "
                               "declared flash end-of-life at the end of 2020, and we cannot guarantee it will "
                               "continue to work in any version after that time.\n"
                               "All support for flash templates will be removed in a future release of CasparCG";

        copy_template_hosts();

        dependencies.producer_registry->register_producer_factory(L"Flash Producer (.ct)", create_ct_producer);
        dependencies.producer_registry->register_producer_factory(L"Flash Producer (.swf)", create_swf_producer);
        dependencies.cg_registry->register_cg_producer(
            L"flash",
            {L".ft", L".ct"},
            [](const spl::shared_ptr<core::frame_producer>& producer) {
                return spl::make_shared<flash_cg_proxy>(producer);
            },
            [](const core::frame_producer_dependencies& dependencies, const std::wstring&) {
                return flash::create_producer(dependencies, {});
            },
            true);
    } else {
        CASPAR_LOG(info) << "Flash support is disabled";
    }
}

std::wstring cg_version() { return L"Unknown"; }

std::wstring version()
{
    std::wstring version = L"Not found";
#ifdef WIN32
    HKEY hkey;

    DWORD dwType, dwSize;
    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, TEXT("SOFTWARE\\Macromedia\\FlashPlayerActiveX"), 0, KEY_QUERY_VALUE, &hkey) ==
        ERROR_SUCCESS) {
        wchar_t ver_str[1024];

        dwType = REG_SZ;
        dwSize = sizeof(ver_str);
        RegQueryValueEx(hkey, TEXT("Version"), nullptr, &dwType, reinterpret_cast<PBYTE>(&ver_str), &dwSize);

        version = ver_str;

        RegCloseKey(hkey);
    }
#endif
    return version;
}

}} // namespace caspar::flash
