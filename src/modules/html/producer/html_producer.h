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
 * Author: Robert Nagy, ronag89@gmail.com
 */

#pragma once

#include <common/memory.h>

#include <core/fwd.h>

#include <string>
#include <vector>

#ifdef WIN32

#undef NOMINMAX
#define NOMINMAX

#include <d3d11_1.h>

#endif

namespace caspar { namespace html {

#ifdef WIN32

class dx11_device final
{
  public:
    dx11_device();
    ~dx11_device();

    dx11_device(const dx11_device&) = delete;
    dx11_device& operator=(const dx11_device&) = delete;

    std::wstring adapter_name() const;

    ID3D11Device* device() const;

    std::shared_ptr<class dx11_context> immedidate_context();

    std::shared_ptr<class dx11_texture2d> open_shared_texture(void* handle);

    std::shared_ptr<class dx11_texture2d> create_texture(int width, int height, DXGI_FORMAT format);

    void copy_texture(ID3D11Texture2D*                              dst,
                      uint32_t                                      dst_x,
                      uint32_t                                      dst_y,
                      const std::shared_ptr<class dx11_texture2d>&  src,
                      uint32_t                                      src_x,
                      uint32_t                                      src_y,
                      uint32_t                                      src_w,
                      uint32_t                                      src_h);

    static const std::shared_ptr<dx11_device>& get_device();

  private:
    struct impl;
    std::unique_ptr<impl> impl_;
};

#endif

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params);
spl::shared_ptr<core::frame_producer> create_cg_producer(const core::frame_producer_dependencies& dependencies,
                                                         const std::vector<std::wstring>&         params);

}} // namespace caspar::html
