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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#include "../StdAfx.h"

#include "strategy_adapters.h"

#include <boost/algorithm/string/replace.hpp>
#include <boost/locale.hpp>
#include <common/log.h>

namespace caspar { namespace IO {

class to_unicode_adapter : public protocol_strategy<char>
{
    std::string                     codepage_;
    protocol_strategy<wchar_t>::ptr unicode_strategy_;

  public:
    to_unicode_adapter(const std::string& codepage, const protocol_strategy<wchar_t>::ptr& unicode_strategy)
        : codepage_(codepage)
        , unicode_strategy_(unicode_strategy)
    {
    }

    void parse(const std::basic_string<char>& data) override
    {
        try {
            // Try multiple UTF-8 conversion methods for better emoji support
            std::wstring utf_data;
            bool conversion_success = false;
            
            // Method 1: Standard conversion with stop method
            try {
                utf_data = boost::locale::conv::to_utf<wchar_t>(data, "UTF-8", boost::locale::conv::method_type::stop);
                conversion_success = true;
            } catch (...) {
                // Fall through to next method
            }
            
            // Method 2: Try with default conversion method
            if (!conversion_success) {
                try {
                    utf_data = boost::locale::conv::to_utf<wchar_t>(data, "UTF-8");
                    conversion_success = true;
                } catch (...) {
                    // Fall through to next method
                }
            }
            
            // Method 3: Use skip method as fallback
            if (!conversion_success) {
                try {
                    utf_data = boost::locale::conv::to_utf<wchar_t>(data, "UTF-8", boost::locale::conv::method_type::skip);
                    conversion_success = true;
                } catch (...) {
                    // Fall through to exception handling
                }
            }
            
            if (conversion_success) {
                unicode_strategy_->parse(utf_data);
            } else {
                // Trigger fallback methods
                throw boost::locale::conv::conversion_error();
            }
        } catch (const boost::locale::conv::conversion_error& e) {
            // Log the conversion error for debugging
            CASPAR_LOG(warning) << L"UTF-8 conversion failed, trying fallback methods. Data size: " << data.size();
            
            // Try fallback approaches to preserve UTF-8 characters
            bool conversion_success = false;
            
            // Fallback 1: Try with stop method using original codepage
            if (!conversion_success) {
                try {
                    auto utf_data = boost::locale::conv::to_utf<wchar_t>(data, codepage_, boost::locale::conv::method_type::stop);
                    unicode_strategy_->parse(utf_data);
                    conversion_success = true;
                } catch (...) {
                    // Continue to next fallback
                }
            }
            
            // Fallback 2: Try direct UTF-8 to UTF-16 conversion
            if (!conversion_success) {
                try {
                    auto fallback_data = boost::locale::conv::utf_to_utf<wchar_t>(data);
                    unicode_strategy_->parse(fallback_data);
                    conversion_success = true;
                } catch (...) {
                    // Continue to next fallback
                }
            }
            
            // Fallback 3: Try with skip method
            if (!conversion_success) {
                try {
                    auto utf_data = boost::locale::conv::to_utf<wchar_t>(data, codepage_, boost::locale::conv::method_type::skip);
                    unicode_strategy_->parse(utf_data);
                    conversion_success = true;
                } catch (...) {
                    // Continue to last resort
                }
            }
            
            // Last resort: Treat as Latin-1
            if (!conversion_success) {
                CASPAR_LOG(warning) << L"All UTF-8 conversion methods failed, treating as Latin-1";
                
                std::wstring fallback_data;
                fallback_data.reserve(data.size());
                for (char c : data) {
                    fallback_data += static_cast<wchar_t>(static_cast<unsigned char>(c));
                }
                unicode_strategy_->parse(fallback_data);
            }
        }
    }
};

class from_unicode_client_connection : public client_connection<wchar_t>
{
    client_connection<char>::ptr client_;
    std::string                  codepage_;

  public:
    from_unicode_client_connection(const client_connection<char>::ptr& client, const std::string& codepage)
        : client_(client)
        , codepage_(codepage)
    {
    }
    ~from_unicode_client_connection() {}

    void send(std::basic_string<wchar_t>&& data, bool skip_log) override
    {
        try {
            auto str = boost::locale::conv::from_utf<wchar_t>(data, codepage_);
            client_->send(std::move(str), skip_log);
            
            if (skip_log)
                return;

            if (data.length() < 512) {
                boost::replace_all(data, L"\n", L"\\n");
                boost::replace_all(data, L"\r", L"\\r");
                CASPAR_LOG(info) << L"Sent message to " << client_->address() << L":" << data;
            } else
                CASPAR_LOG(info) << L"Sent more than 512 bytes to " << client_->address();
        } catch (const std::exception& e) {
            CASPAR_LOG(error) << L"Failed to send message to " << client_->address() << L": " << e.what();
            // Try to send a simple error message
            try {
                std::string simple_error = "500 ENCODING ERROR\r\n";
                client_->send(std::move(simple_error), true);
            } catch (...) {
                // If even that fails, disconnect the client
                CASPAR_LOG(error) << L"Complete send failure, disconnecting client " << client_->address();
                client_->disconnect();
            }
        } catch (...) {
            CASPAR_LOG(error) << L"Failed to send message to " << client_->address() << L" with unknown error";
            // Try to disconnect gracefully
            try {
                client_->disconnect();
            } catch (...) {
                // Nothing more we can do
            }
        }
    }

    void disconnect() override { client_->disconnect(); }

    std::wstring address() const override { return client_->address(); }

    void add_lifecycle_bound_object(const std::wstring& key, const std::shared_ptr<void>& lifecycle_bound) override
    {
        client_->add_lifecycle_bound_object(key, lifecycle_bound);
    }
    std::shared_ptr<void> remove_lifecycle_bound_object(const std::wstring& key) override
    {
        return client_->remove_lifecycle_bound_object(key);
    }
};

to_unicode_adapter_factory::to_unicode_adapter_factory(
    const std::string&                             codepage,
    const protocol_strategy_factory<wchar_t>::ptr& unicode_strategy_factory)
    : codepage_(codepage)
    , unicode_strategy_factory_(unicode_strategy_factory)
{
}

protocol_strategy<char>::ptr to_unicode_adapter_factory::create(const client_connection<char>::ptr& client_connection)
{
    auto client = spl::make_shared<from_unicode_client_connection>(client_connection, codepage_);

    return spl::make_shared<to_unicode_adapter>(codepage_, unicode_strategy_factory_->create(client));
}

}} // namespace caspar::IO
