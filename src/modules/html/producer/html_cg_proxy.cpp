/*
 * Copyright 2013 Sveriges Television AB http://casparcg.com/
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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#include "html_cg_proxy.h"

#include <future>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/algorithm/string/trim.hpp>
#include <boost/format.hpp>

namespace caspar { namespace html {

struct html_cg_proxy::impl
{
    spl::shared_ptr<core::frame_producer> producer;
};

html_cg_proxy::html_cg_proxy(const spl::shared_ptr<core::frame_producer>& producer)
    : impl_(new impl{producer})
{
}

html_cg_proxy::~html_cg_proxy() {}

void html_cg_proxy::add(int                 layer,
                        const std::wstring& template_name,
                        bool                play_on_load,
                        const std::wstring& start_from_label,
                        const std::wstring& data)
{
    update(layer, data);

    if (play_on_load)
        play(layer);
}

void html_cg_proxy::remove(int layer) { impl_->producer->call({L"remove()"}); }

void html_cg_proxy::play(int layer) { impl_->producer->call({L"play()"}); }

void html_cg_proxy::stop(int layer) { impl_->producer->call({L"stop()"}); }

void html_cg_proxy::next(int layer) { impl_->producer->call({L"next()"}); }

void html_cg_proxy::update(int layer, const std::wstring& data)
{
    impl_->producer->call({(boost::wformat(L"update(\"%1%\")") %
                            boost::algorithm::replace_all_copy(
                                boost::algorithm::trim_copy_if(data, boost::is_any_of(" \"")), "\"", "\\\""))
                               .str()});
}

std::wstring html_cg_proxy::invoke(int layer, const std::wstring& label)
{
    auto function_call = boost::algorithm::trim_copy_if(label, boost::is_any_of(" \""));

    // Append empty () if no parameter list has been given
    auto javascript = boost::ends_with(function_call, ")") ? function_call : function_call + L"()";
    return impl_->producer->call({javascript}).get();
}

}} // namespace caspar::html
