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

#pragma once

#include <boost/algorithm/string/split.hpp>

#include "protocol_strategy.h"
#include "ProtocolStrategy.h"

namespace caspar { namespace IO {

/**
 * A protocol strategy factory adapter for converting incoming data from a
 * specific codepage to utf-16. The client_connection will do the reversed
 * conversion.
 *
 * The adapter is not safe if the codepage contains multibyte-characters and
 * the data is chunked with potentially incomplete characters, therefore it
 * must be wrapped in an adapter providing complete chunks like
 * delimiter_based_chunking_strategy_factory.
 */
class to_unicode_adapter_factory : public protocol_strategy_factory<char>
{
	std::string codepage_;
	protocol_strategy_factory<wchar_t>::ptr unicode_strategy_factory_;
public:
	to_unicode_adapter_factory(
			const std::string& codepage, 
			const protocol_strategy_factory<wchar_t>::ptr& unicode_strategy_factory);

	virtual protocol_strategy<char>::ptr create(
			const client_connection<char>::ptr& client_connection);
};

/**
 * Protocol strategy adapter for ensuring that only complete chunks or
 * "packets" are delivered to the wrapped strategy. The chunks are determined
 * by a given delimiter.
 */
template<class CharT>
class delimiter_based_chunking_strategy : public protocol_strategy<CharT>
{
	std::basic_string<CharT> delimiter_;
	std::basic_string<CharT> input_;
	protocol_strategy<CharT>::ptr strategy_;
public:
	delimiter_based_chunking_strategy(
			const std::basic_string<CharT>& delimiter, 
			const protocol_strategy<CharT>::ptr& strategy)
		: delimiter_(delimiter)
		, strategy_(strategy)
	{
	}

	virtual void parse(const std::basic_string<char>& data)
	{
		input_ += data;

		std::vector<std::basic_string<CharT>> split;
		boost::iter_split(split, input_, boost::algorithm::first_finder(delimiter_));

		input_ = std::move(split.back());
		split.pop_back();

		BOOST_FOREACH(auto cmd, split)
		{
			// TODO: perhaps it would be better to not append the delimiter.
			strategy_->parse(cmd + delimiter_);
		}
	}
};

template<class CharT>
class delimiter_based_chunking_strategy_factory 
	: public protocol_strategy_factory<CharT>
{
	std::basic_string<CharT> delimiter_;
	protocol_strategy_factory<CharT>::ptr strategy_factory_;
public:
	delimiter_based_chunking_strategy_factory(
			const std::basic_string<CharT>& delimiter, 
			const protocol_strategy_factory<CharT>::ptr& strategy_factory)
		: delimiter_(delimiter)
		, strategy_factory_(strategy_factory)
	{
	}

	virtual typename protocol_strategy<CharT>::ptr create(
			const typename client_connection<CharT>::ptr& client_connection)
	{
		return spl::make_shared<delimiter_based_chunking_strategy<CharT>>(
			delimiter_, strategy_factory_->create(client_connection));
	}
};

/**
 * Adapts an IProtocolStrategy to be used as a
 * protocol_strategy_factory<wchar_t>.
 *
 * Use wrap_legacy_protocol() to wrap it as a protocol_strategy_factory<char>
 * for use directly by the async event server.
 */
class legacy_strategy_adapter_factory 
	: public protocol_strategy_factory<wchar_t>
{
	ProtocolStrategyPtr strategy_;
public:
	legacy_strategy_adapter_factory(const ProtocolStrategyPtr& strategy);

	virtual protocol_strategy<wchar_t>::ptr create(
			const client_connection<wchar_t>::ptr& client_connection);
};

/**
 * Wraps an IProtocolStrategy in a legacy_strategy_adapter_factory, wrapped in
 * a to_unicode_adapter_factory (using the codepage reported by the 
 * IProtocolStrategy) wrapped in a delimiter_based_chunking_strategy_factory
 * with the given delimiter string.
 *
 * @param delimiter The delimiter to use to separate messages.
 * @param strategy  The legacy protocol strategy (the same instance will serve
 *                  all connections).
 *
 * @return the adapted strategy.
 */
protocol_strategy_factory<char>::ptr wrap_legacy_protocol(
		const std::string& delimiter, 
		const ProtocolStrategyPtr& strategy);

}}
