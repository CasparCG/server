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
 */

#pragma once

#include <common/memory.h>

#include <core/producer/cg_proxy.h>

namespace caspar { namespace html {

class html_cg_proxy : public core::cg_proxy
{
  public:
    explicit html_cg_proxy(const spl::shared_ptr<core::frame_producer>& producer);
    ~html_cg_proxy();

    void         add(int                 layer,
                     const std::wstring& template_name,
                     bool                play_on_load,
                     const std::wstring& start_from_label,
                     const std::wstring& data) override;
    void         remove(int layer) override;
    void         play(int layer) override;
    void         stop(int layer) override;
    void         next(int layer) override;
    void         update(int layer, const std::wstring& data) override;
    std::wstring invoke(int layer, const std::wstring& label) override;

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;
};

}} // namespace caspar::html
