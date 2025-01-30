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

#include "frame_consumer.h"

#include <common/future.h>

#include <core/frame/frame.h>
#include <core/video_format.h>

#include <future>

namespace caspar { namespace core {

const spl::shared_ptr<frame_consumer>& frame_consumer::empty()
{
    class empty_frame_consumer : public frame_consumer
    {
      public:
        std::future<bool> send(const core::video_field field, const_frame) override { return make_ready_future(false); }
        void              initialize(const video_format_desc&, int) override {}
        std::wstring      print() const override { return L"empty"; }
        std::wstring      name() const override { return L"empty"; }
        bool              has_synchronization_clock() const override { return false; }
        int               index() const override { return -1; }
        core::monitor::state state() const override
        {
            static const monitor::state empty;
            return empty;
        }
    };
    static spl::shared_ptr<frame_consumer> consumer = spl::make_shared<empty_frame_consumer>();
    return consumer;
}

}} // namespace caspar::core
