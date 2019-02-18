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

#pragma once

#include "frame_producer.h"

#include <common/memory.h>

#include <functional>
#include <set>
#include <string>

namespace caspar { namespace core {

class cg_proxy
{
  public:
    static const unsigned int DEFAULT_LAYER = 9999;

    virtual ~cg_proxy() {}

    virtual void         add(int                 layer,
                             const std::wstring& template_name,
                             bool                play_on_load,
                             const std::wstring& start_from_label = L"",
                             const std::wstring& data             = L"")          = 0;
    virtual void         remove(int layer)                            = 0;
    virtual void         play(int layer)                              = 0;
    virtual void         stop(int layer)                              = 0;
    virtual void         next(int layer)                              = 0;
    virtual void         update(int layer, const std::wstring& data)  = 0;
    virtual std::wstring invoke(int layer, const std::wstring& label) = 0;

    static const spl::shared_ptr<cg_proxy>& empty();
};

using cg_proxy_factory = std::function<spl::shared_ptr<cg_proxy>(const spl::shared_ptr<frame_producer>& producer)>;
using cg_producer_factory =
    std::function<spl::shared_ptr<frame_producer>(const frame_producer_dependencies& dependencies,
                                                  const std::wstring&                filename)>;

class cg_producer_registry
{
  public:
    cg_producer_registry();

    void register_cg_producer(std::wstring           cg_producer_name,
                              std::set<std::wstring> file_extensions,
                              cg_proxy_factory       proxy_factory,
                              cg_producer_factory    producer_factory,
                              bool                   reusable_producer_instance);

    spl::shared_ptr<frame_producer> create_producer(const frame_producer_dependencies& dependencies,
                                                    const std::wstring&                filename) const;

    spl::shared_ptr<cg_proxy> get_proxy(const spl::shared_ptr<frame_producer>& producer) const;
    spl::shared_ptr<cg_proxy> get_proxy(const spl::shared_ptr<video_channel>& video_channel, int render_layer) const;
    spl::shared_ptr<cg_proxy> get_or_create_proxy(const spl::shared_ptr<video_channel>& video_channel,
                                                  const frame_producer_dependencies&    dependencies,
                                                  int                                   render_layer,
                                                  const std::wstring&                   filename) const;

    bool         is_cg_extension(const std::wstring& extension) const;
    std::wstring get_cg_producer_name(const std::wstring& filename) const;

  private:
    struct impl;
    spl::shared_ptr<impl> impl_;

    cg_producer_registry(const cg_producer_registry&) = delete;
    cg_producer_registry& operator=(const cg_producer_registry&) = delete;
};

void init_cg_proxy_as_producer(core::module_dependencies dependencies);

}} // namespace caspar::core
