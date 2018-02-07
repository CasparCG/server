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
        auto utf_data = boost::locale::conv::to_utf<wchar_t>(data, codepage_);

        unicode_strategy_->parse(utf_data);
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

class legacy_strategy_adapter : public protocol_strategy<wchar_t>
{
    ProtocolStrategyPtr strategy_;
    ClientInfoPtr       client_info_;

  public:
    legacy_strategy_adapter(const ProtocolStrategyPtr&             strategy,
                            const client_connection<wchar_t>::ptr& client_connection)
        : strategy_(strategy)
        , client_info_(client_connection)
    {
    }
    ~legacy_strategy_adapter() {}

    void parse(const std::basic_string<wchar_t>& data) override { strategy_->Parse(data, client_info_); }
};

legacy_strategy_adapter_factory::legacy_strategy_adapter_factory(const ProtocolStrategyPtr& strategy)
    : strategy_(strategy)
{
}

protocol_strategy<wchar_t>::ptr
legacy_strategy_adapter_factory::create(const client_connection<wchar_t>::ptr& client_connection)
{
    return spl::make_shared<legacy_strategy_adapter>(strategy_, client_connection);
}

protocol_strategy_factory<char>::ptr wrap_legacy_protocol(const std::string&         delimiter,
                                                          const ProtocolStrategyPtr& strategy)
{
    return spl::make_shared<delimiter_based_chunking_strategy_factory<char>>(
        delimiter,
        spl::make_shared<to_unicode_adapter_factory>(strategy->GetCodepage(),
                                                     spl::make_shared<legacy_strategy_adapter_factory>(strategy)));
}

}} // namespace caspar::IO
