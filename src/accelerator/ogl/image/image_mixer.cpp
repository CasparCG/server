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
#include "image_mixer.h"

#include "image_kernel.h"

#include "../util/buffer.h"
#include "../util/device.h"
#include "../util/texture.h"

#include <common/array.h>
#include <common/future.h>
#include <common/log.h>

#include <core/frame/frame.h>
#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>
#include <core/video_format.h>

#include <GL/glew.h>

#ifdef WIN32
#include "../../d3d/d3d_texture2d.h"
#endif

#include <boost/any.hpp>

#include <algorithm>
#include <vector>

namespace caspar { namespace accelerator { namespace ogl {

using future_texture = std::shared_future<std::shared_ptr<texture>>;

struct item
{
    core::pixel_format_desc     pix_desc = core::pixel_format::invalid;
    std::vector<future_texture> textures;
    core::image_transform       transform;
    core::frame_geometry        geometry = core::frame_geometry::get_default();
};

struct layer
{
    std::vector<layer> sublayers;
    std::vector<item>  items;
    core::blend_mode   blend_mode;

    explicit layer(core::blend_mode blend_mode)
        : blend_mode(blend_mode)
    {
    }
};

class image_renderer
{
    spl::shared_ptr<device> ogl_;
    image_kernel            kernel_;

  public:
    explicit image_renderer(const spl::shared_ptr<device>& ogl)
        : ogl_(ogl)
        , kernel_(ogl_)
    {
    }

    std::future<array<const std::uint8_t>> operator()(std::vector<layer>             layers,
                                                      const core::video_format_desc& format_desc)
    {
        if (layers.empty()) { // Bypass GPU with empty frame.
            static const std::vector<uint8_t> buffer(4096 * 4096 * 4, 0);
            return make_ready_future(array<const std::uint8_t>(buffer.data(), format_desc.size, true));
        }

        return flatten(ogl_->dispatch_async([=]() mutable -> std::shared_future<array<const std::uint8_t>> {
            auto target_texture = ogl_->create_texture(format_desc.width, format_desc.height, 4);

            draw(target_texture, std::move(layers), format_desc);

            return ogl_->copy_async(target_texture);
        }));
    }

  private:
    void draw(std::shared_ptr<texture>&      target_texture,
              std::vector<layer>             layers,
              const core::video_format_desc& format_desc)
    {
        std::shared_ptr<texture> layer_key_texture;

        for (auto& layer : layers) {
            draw(target_texture, layer.sublayers, format_desc);
            draw(target_texture, std::move(layer), layer_key_texture, format_desc);
        }
    }

    void draw(std::shared_ptr<texture>&      target_texture,
              layer                          layer,
              std::shared_ptr<texture>&      layer_key_texture,
              const core::video_format_desc& format_desc)
    {
        if (layer.items.empty())
            return;

        std::shared_ptr<texture> local_key_texture;
        std::shared_ptr<texture> local_mix_texture;

        if (layer.blend_mode != core::blend_mode::normal) {
            auto layer_texture = ogl_->create_texture(target_texture->width(), target_texture->height(), 4);

            for (auto& item : layer.items)
                draw(layer_texture,
                     std::move(item),
                     layer_key_texture,
                     local_key_texture,
                     local_mix_texture,
                     format_desc);

            draw(layer_texture, std::move(local_mix_texture), core::blend_mode::normal);
            draw(target_texture, std::move(layer_texture), layer.blend_mode);
        } else // fast path
        {
            for (auto& item : layer.items)
                draw(target_texture,
                     std::move(item),
                     layer_key_texture,
                     local_key_texture,
                     local_mix_texture,
                     format_desc);

            draw(target_texture, std::move(local_mix_texture), core::blend_mode::normal);
        }

        layer_key_texture = std::move(local_key_texture);
    }

    void draw(std::shared_ptr<texture>&      target_texture,
              item                           item,
              std::shared_ptr<texture>&      layer_key_texture,
              std::shared_ptr<texture>&      local_key_texture,
              std::shared_ptr<texture>&      local_mix_texture,
              const core::video_format_desc& format_desc)
    {
        draw_params draw_params;
        draw_params.pix_desc  = std::move(item.pix_desc);
        draw_params.transform = std::move(item.transform);
        draw_params.geometry  = item.geometry;
        draw_params.aspect_ratio =
            static_cast<double>(format_desc.square_width) / static_cast<double>(format_desc.square_height);

        for (auto& future_texture : item.textures) {
            draw_params.textures.push_back(spl::make_shared_ptr(future_texture.get()));
        }

        if (item.transform.is_key) {
            local_key_texture = local_key_texture
                                    ? local_key_texture
                                    : ogl_->create_texture(target_texture->width(), target_texture->height(), 1);

            draw_params.background = local_key_texture;
            draw_params.local_key  = nullptr;
            draw_params.layer_key  = nullptr;

            kernel_.draw(std::move(draw_params));
        } else if (item.transform.is_mix) {
            local_mix_texture = local_mix_texture
                                    ? local_mix_texture
                                    : ogl_->create_texture(target_texture->width(), target_texture->height(), 4);

            draw_params.background = local_mix_texture;
            draw_params.local_key  = std::move(local_key_texture);
            draw_params.layer_key  = layer_key_texture;

            draw_params.keyer = keyer::additive;

            kernel_.draw(std::move(draw_params));
        } else {
            draw(target_texture, std::move(local_mix_texture), core::blend_mode::normal);

            draw_params.background = target_texture;
            draw_params.local_key  = std::move(local_key_texture);
            draw_params.layer_key  = layer_key_texture;

            kernel_.draw(std::move(draw_params));
        }
    }

    void draw(std::shared_ptr<texture>&  target_texture,
              std::shared_ptr<texture>&& source_buffer,
              core::blend_mode           blend_mode = core::blend_mode::normal)
    {
        if (!source_buffer)
            return;

        draw_params draw_params;
        draw_params.pix_desc.format = core::pixel_format::bgra;
        draw_params.pix_desc.planes = {
            core::pixel_format_desc::plane(source_buffer->width(), source_buffer->height(), 4)};
        draw_params.textures   = {spl::make_shared_ptr(source_buffer)};
        draw_params.transform  = core::image_transform();
        draw_params.blend_mode = blend_mode;
        draw_params.background = target_texture;
        draw_params.geometry   = core::frame_geometry::get_default();

        kernel_.draw(std::move(draw_params));
    }
};

struct image_mixer::impl
    : public core::frame_factory
    , public std::enable_shared_from_this<impl>
{
    spl::shared_ptr<device>            ogl_;
    image_renderer                     renderer_;
    std::vector<core::image_transform> transform_stack_;
    std::vector<layer>                 layers_; // layer/stream/items
    std::vector<layer*>                layer_stack_;

  public:
    impl(const spl::shared_ptr<device>& ogl, int channel_id)
        : ogl_(ogl)
        , renderer_(ogl)
        , transform_stack_(1)
    {
        CASPAR_LOG(info) << L"Initialized OpenGL Accelerated GPU Image Mixer for channel " << channel_id;
    }

    void push(const core::frame_transform& transform)
    {
        auto previous_layer_depth = transform_stack_.back().layer_depth;
        transform_stack_.push_back(transform_stack_.back() * transform.image_transform);
        auto new_layer_depth = transform_stack_.back().layer_depth;

        if (previous_layer_depth < new_layer_depth) {
            layer new_layer(transform_stack_.back().blend_mode);

            if (layer_stack_.empty()) {
                layers_.push_back(std::move(new_layer));
                layer_stack_.push_back(&layers_.back());
            } else {
                layer_stack_.back()->sublayers.push_back(std::move(new_layer));
                layer_stack_.push_back(&layer_stack_.back()->sublayers.back());
            }
        }
    }

    void visit(const core::const_frame& frame)
    {
        if (frame.pixel_format_desc().format == core::pixel_format::invalid)
            return;

        if (frame.pixel_format_desc().planes.empty())
            return;

        item item;
        item.pix_desc  = frame.pixel_format_desc();
        item.transform = transform_stack_.back();
        item.geometry  = frame.geometry();

        auto textures_ptr = boost::any_cast<std::shared_ptr<std::vector<future_texture>>>(frame.opaque());

        if (textures_ptr) {
            item.textures = *textures_ptr;
        } else {
            for (int n = 0; n < static_cast<int>(item.pix_desc.planes.size()); ++n) {
                item.textures.emplace_back(ogl_->copy_async(frame.image_data(n),
                                                            item.pix_desc.planes[n].width,
                                                            item.pix_desc.planes[n].height,
                                                            item.pix_desc.planes[n].stride));
            }
        }

        layer_stack_.back()->items.push_back(item);
    }

    void pop()
    {
        transform_stack_.pop_back();
        layer_stack_.resize(transform_stack_.back().layer_depth);
    }

    std::future<array<const std::uint8_t>> render(const core::video_format_desc& format_desc)
    {
        return renderer_(std::move(layers_), format_desc);
    }

    core::mutable_frame create_frame(const void* tag, const core::pixel_format_desc& desc) override
    {
        std::vector<array<std::uint8_t>> image_data;
        for (auto& plane : desc.planes) {
            image_data.push_back(ogl_->create_array(plane.size));
        }

        std::weak_ptr<image_mixer::impl> weak_self = shared_from_this();
        return core::mutable_frame(
            tag,
            std::move(image_data),
            array<int32_t>{},
            desc,
            [weak_self, desc](std::vector<array<const std::uint8_t>> image_data) -> boost::any {
                auto self = weak_self.lock();
                if (!self) {
                    return boost::any{};
                }
                std::vector<future_texture> textures;
                for (int n = 0; n < static_cast<int>(desc.planes.size()); ++n) {
                    textures.emplace_back(self->ogl_->copy_async(
                        image_data[n], desc.planes[n].width, desc.planes[n].height, desc.planes[n].stride));
                }
                return std::make_shared<decltype(textures)>(std::move(textures));
            });
    }

#ifdef WIN32
    core::const_frame
    import_d3d_texture(const void* tag, const std::shared_ptr<d3d::d3d_texture2d>& d3d_texture, bool vflip) override
    {
        // map directx texture with wgl texture
        if (d3d_texture->gl_texture_id() == 0)
            ogl_->dispatch_sync([=] { d3d_texture->gen_gl_texture(ogl_->d3d_interop()); });

        // copy directx texture to gl texture
        auto gl_texture = ogl_->dispatch_sync([=] {
            return ogl_->copy_async(d3d_texture->gl_texture_id(), d3d_texture->width(), d3d_texture->height(), 4);
        });

        // make gl texture to draw
        std::vector<future_texture> textures{make_ready_future(gl_texture.get())};

        std::weak_ptr<image_mixer::impl> weak_self = shared_from_this();
        core::pixel_format_desc          desc(core::pixel_format::bgra);
        desc.planes.push_back(core::pixel_format_desc::plane(d3d_texture->width(), d3d_texture->height(), 4));
        auto frame = core::mutable_frame(
            tag,
            std::vector<array<uint8_t>>{},
            array<int32_t>{},
            desc,
            [weak_self, texs = std::move(textures)](std::vector<array<const std::uint8_t>> image_data) -> boost::any {
                auto self = weak_self.lock();
                if (!self) {
                    return boost::any{};
                }

                return std::make_shared<decltype(textures)>(std::move(texs));
            });

        if (vflip) {
            frame.geometry() = core::frame_geometry::get_default_vflip();
        }

        return core::const_frame(std::move(frame));
    }
#endif
};

image_mixer::image_mixer(const spl::shared_ptr<device>& ogl, int channel_id)
    : impl_(std::make_unique<impl>(ogl, channel_id))
{
}
image_mixer::~image_mixer() {}
void image_mixer::push(const core::frame_transform& transform) { impl_->push(transform); }
void image_mixer::visit(const core::const_frame& frame) { impl_->visit(frame); }
void image_mixer::pop() { impl_->pop(); }
std::future<array<const std::uint8_t>> image_mixer::operator()(const core::video_format_desc& format_desc)
{
    return impl_->render(format_desc);
}
core::mutable_frame image_mixer::create_frame(const void* tag, const core::pixel_format_desc& desc)
{
    return impl_->create_frame(tag, desc);
}

#ifdef WIN32
core::const_frame
image_mixer::import_d3d_texture(const void* tag, const std::shared_ptr<d3d::d3d_texture2d>& d3d_texture, bool vflip)
{
    return impl_->import_d3d_texture(tag, d3d_texture, vflip);
}
#endif
}}} // namespace caspar::accelerator::ogl
