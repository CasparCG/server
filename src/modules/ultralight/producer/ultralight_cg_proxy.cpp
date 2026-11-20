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
 * Author: Ole Kristensen, Den Frie Vilje ApS, ole@kristensen.name
 */

#include "ultralight_cg_proxy.h"

#include <common/future.h>
#include <common/utf.h>

#include <boost/algorithm/string.hpp>

namespace caspar { namespace ultralight {

ultralight_cg_proxy::ultralight_cg_proxy(const spl::shared_ptr<core::frame_producer>& producer)
    : producer_(producer)
{
}

void ultralight_cg_proxy::add(int                 layer,
                               const std::wstring& template_name,
                               bool                play_on_load,
                               const std::wstring& start_from_label,
                               const std::wstring& data)
{
    if (!data.empty())
        update(layer, data);

    if (play_on_load)
        play(layer);
}

void ultralight_cg_proxy::remove(int layer)
{
    producer_->call({L"remove()"});
}

void ultralight_cg_proxy::play(int layer)
{
    producer_->call({L"play()"});
}

void ultralight_cg_proxy::stop(int layer)
{
    producer_->call({L"stop()"});
}

void ultralight_cg_proxy::next(int layer)
{
    producer_->call({L"next()"});
}

void ultralight_cg_proxy::update(int layer, const std::wstring& data)
{
    // Escape the data string for safe JavaScript injection.
    // Mirrors html_cg_proxy's escaping logic.
    std::wstring escaped = data;
    boost::replace_all(escaped, L"\\", L"\\\\");
    boost::replace_all(escaped, L"\"", L"\\\"");
    boost::replace_all(escaped, L"\n", L"\\n");
    boost::replace_all(escaped, L"\r", L"\\r");
    boost::replace_all(escaped, L"\t", L"\\t");

    producer_->call({L"update(\"" + escaped + L"\")"});
}

std::wstring ultralight_cg_proxy::invoke(int layer, const std::wstring& label)
{
    auto javascript = label;

    // Append () if the label doesn't already end with it
    if (!boost::algorithm::ends_with(javascript, L")"))
        javascript += L"()";

    return producer_->call({javascript}).get();
}

}} // namespace caspar::ultralight
