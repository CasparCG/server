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

#include "../stdafx.h"

#include "strategy_adapters.h"

#include <boost/locale.hpp>

namespace caspar { namespace IO {

class to_unicode_adapter : public protocol_strategy<char>
{
	std::string codepage_;
	protocol_strategy<wchar_t>::ptr unicode_strategy_;
public:
	to_unicode_adapter(
			const std::string& codepage, 
			const protocol_strategy<wchar_t>::ptr& unicode_strategy)
		: codepage_(codepage)
		, unicode_strategy_(unicode_strategy)
	{
	}

	virtual void parse(const std::basic_string<char>& data)
	{
		auto utf_data = boost::locale::conv::to_utf<wchar_t>(data, codepage_);

		unicode_strategy_->parse(utf_data);
	}
};

class from_unicode_client_connection : public client_connection<wchar_t>
{
	client_connection<char>::ptr client_;
	std::string codepage_;
public:
	from_unicode_client_connection(
			const client_connection<char>::ptr& client, const std::string& codepage)
		: client_(client)
		, codepage_(codepage)
	{
	}

	virtual void send(std::basic_string<wchar_t>&& data)
	{
		auto str = boost::locale::conv::from_utf<wchar_t>(std::move(data), codepage_);

		client_->send(std::move(str));
	}

	virtual void disconnect()
	{
		client_->disconnect();
	}

	virtual std::wstring print() const
	{
		return client_->print();
	}
};

to_unicode_adapter_factory::to_unicode_adapter_factory(
		const std::string& codepage, 
		const protocol_strategy_factory<wchar_t>::ptr& unicode_strategy_factory)
	: codepage_(codepage)
	, unicode_strategy_factory_(unicode_strategy_factory)
{
}

protocol_strategy<char>::ptr to_unicode_adapter_factory::create(
	const client_connection<char>::ptr& client_connection)
{
	auto client = spl::make_shared<from_unicode_client_connection>(client_connection, codepage_);

	return spl::make_shared<to_unicode_adapter>(codepage_, unicode_strategy_factory_->create(client));
}

class legacy_client_info : public ClientInfo
{
	client_connection<wchar_t>::ptr client_connection_;
public:
	legacy_client_info(const client_connection<wchar_t>::ptr& client_connection)
		: client_connection_(client_connection)
	{
	}

	virtual void Disconnect()
	{
		client_connection_->disconnect();
	}

	virtual void Send(const std::wstring& data)
	{
		client_connection_->send(std::wstring(data));
	}

	virtual std::wstring print() const 
	{
		return client_connection_->print();
	}
};

class legacy_strategy_adapter : public protocol_strategy<wchar_t>
{
	ProtocolStrategyPtr strategy_;
	ClientInfoPtr client_info_;
public:
	legacy_strategy_adapter(
			const ProtocolStrategyPtr& strategy, 
			const client_connection<wchar_t>::ptr& client_connection)
		: strategy_(strategy)
		, client_info_(std::make_shared<legacy_client_info>(client_connection))
	{
	}

	virtual void parse(const std::basic_string<wchar_t>& data)
	{
		auto p = data.c_str();
		strategy_->Parse(p, static_cast<int>(data.length()), client_info_);
	}
};

legacy_strategy_adapter_factory::legacy_strategy_adapter_factory(
		const ProtocolStrategyPtr& strategy)
	: strategy_(strategy)
{
}

protocol_strategy<wchar_t>::ptr legacy_strategy_adapter_factory::create(
		const client_connection<wchar_t>::ptr& client_connection)
{
	return spl::make_shared<legacy_strategy_adapter>(strategy_, client_connection);
}

protocol_strategy_factory<char>::ptr wrap_legacy_protocol(
		const std::string& delimiter, 
		const ProtocolStrategyPtr& strategy)
{
	return spl::make_shared<delimiter_based_chunking_strategy_factory<char>>(
			delimiter,
			spl::make_shared<to_unicode_adapter_factory>(
					strategy->GetCodepage(),
					spl::make_shared<legacy_strategy_adapter_factory>(strategy)));
}

}}
