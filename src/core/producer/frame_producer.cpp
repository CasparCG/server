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

#include "../StdAfx.h"

#include "cg_proxy.h"
#include "frame_producer.h"

#include <common/except.h>
#include <common/future.h>
#include <common/memory.h>

#include <boost/algorithm/string/predicate.hpp>

namespace caspar { namespace core {

frame_producer_dependencies::frame_producer_dependencies(
    const spl::shared_ptr<core::frame_factory>&           frame_factory,
    const std::vector<spl::shared_ptr<video_channel>>&    channels,
    const video_format_repository&                        format_repository,
    const video_format_desc&                              format_desc,
    const spl::shared_ptr<const frame_producer_registry>& producer_registry,
    const spl::shared_ptr<const cg_producer_registry>&    cg_registry)
    : frame_factory(frame_factory)
    , channels(channels)
    , format_repository(format_repository)
    , format_desc(format_desc)
    , producer_registry(producer_registry)
    , cg_registry(cg_registry)
{
}

const spl::shared_ptr<frame_producer>& frame_producer::empty()
{
    class empty_frame_producer : public frame_producer
    {
      public:
        empty_frame_producer() {}

        draw_frame   receive_impl(const core::video_field field, int nb_samples) override { return draw_frame{}; }
        uint32_t     nb_frames() const override { return 0; }
        std::wstring print() const override { return L"empty"; }
        std::wstring name() const override { return L"empty"; }
        uint32_t     frame_number() const override { return 0; }
        std::future<std::wstring> call(const std::vector<std::wstring>& params) override
        {
            CASPAR_LOG(warning) << L" Cannot call on empty frame_producer";
            return make_ready_future(std::wstring());
        }
        draw_frame           last_frame(const core::video_field field) override { return draw_frame{}; }
        draw_frame           first_frame(const core::video_field field) override { return draw_frame{}; }
        core::monitor::state state() const override
        {
            static const monitor::state empty;
            return empty;
        }

        bool is_ready() override { return true; }
    };

    static spl::shared_ptr<frame_producer> producer = spl::make_shared<empty_frame_producer>();
    return producer;
}

}} // namespace caspar::core
