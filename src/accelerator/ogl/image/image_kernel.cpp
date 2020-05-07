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
#include "image_kernel.h"

#include "image_shader.h"

#include "../util/device.h"
#include "../util/shader.h"
#include "../util/texture.h"

#include <common/assert.h>
#include <common/gl/gl_check.h>

#include <core/frame/frame_transform.h>
#include <core/frame/pixel_format.h>

#include <boost/algorithm/cxx11/all_of.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <GL/glew.h>

#include <array>
#include <cmath>

namespace caspar { namespace accelerator { namespace ogl {

// http://stackoverflow.com/questions/563198/how-do-you-detect-where-two-line-segments-intersect
bool get_line_intersection(double  p0_x,
                           double  p0_y,
                           double  p1_x,
                           double  p1_y,
                           double  p2_x,
                           double  p2_y,
                           double  p3_x,
                           double  p3_y,
                           double& result_x,
                           double& result_y)
{
    double s1_x = p1_x - p0_x;
    double s1_y = p1_y - p0_y;
    double s2_x = p3_x - p2_x;
    double s2_y = p3_y - p2_y;

    double s = (-s1_y * (p0_x - p2_x) + s1_x * (p0_y - p2_y)) / (-s2_x * s1_y + s1_x * s2_y);
    double t = (s2_x * (p0_y - p2_y) - s2_y * (p0_x - p2_x)) / (-s2_x * s1_y + s1_x * s2_y);

    if (s >= 0 && s <= 1 && t >= 0 && t <= 1) {
        // Collision detected
        result_x = p0_x + t * s1_x;
        result_y = p0_y + t * s1_y;

        return true;
    }

    return false; // No collision
}

double hypotenuse(double x1, double y1, double x2, double y2)
{
    auto x = x2 - x1;
    auto y = y2 - y1;

    return std::sqrt(x * x + y * y);
}

double calc_q(double close_diagonal, double distant_diagonal)
{
    return (close_diagonal + distant_diagonal) / distant_diagonal;
}

bool is_above_screen(double y) { return y < 0.0; }

bool is_below_screen(double y) { return y > 1.0; }

bool is_left_of_screen(double x) { return x < 0.0; }

bool is_right_of_screen(double x) { return x > 1.0; }

bool is_outside_screen(const std::vector<core::frame_geometry::coord>& coords)
{
    auto x_coords =
        coords | boost::adaptors::transformed([](const core::frame_geometry::coord& c) { return c.vertex_x; });
    auto y_coords =
        coords | boost::adaptors::transformed([](const core::frame_geometry::coord& c) { return c.vertex_y; });

    return boost::algorithm::all_of(x_coords, &is_left_of_screen) ||
           boost::algorithm::all_of(x_coords, &is_right_of_screen) ||
           boost::algorithm::all_of(y_coords, &is_above_screen) || boost::algorithm::all_of(y_coords, &is_below_screen);
}

struct image_kernel::impl
{
    spl::shared_ptr<device> ogl_;
    spl::shared_ptr<shader> shader_;
    GLuint                  vao_;
    GLuint                  vbo_;

    explicit impl(const spl::shared_ptr<device>& ogl)
        : ogl_(ogl)
        , shader_(ogl_->dispatch_sync([&] { return get_image_shader(ogl); }))
    {
        ogl_->dispatch_sync([&] {
            GL(glGenVertexArrays(1, &vao_));
            GL(glGenBuffers(1, &vbo_));
        });
    }

    ~impl()
    {
        ogl_->dispatch_sync([&] {
            GL(glDeleteVertexArrays(1, &vao_));
            GL(glDeleteBuffers(1, &vbo_));
        });
    }

    void draw(draw_params params)
    {
        static const double epsilon = 0.001;

        CASPAR_ASSERT(params.pix_desc.planes.size() == params.textures.size());

        if (params.textures.empty() || !params.background) {
            return;
        }

        if (params.transform.opacity < epsilon) {
            return;
        }

        auto coords = params.geometry.data();

        if (coords.empty()) {
            return;
        }

        // Calculate transforms
        auto f_p = params.transform.fill_translation;
        auto f_s = params.transform.fill_scale;

        bool is_default_geometry = boost::equal(coords, core::frame_geometry::get_default().data()) ||
                                   boost::equal(coords, core::frame_geometry::get_default_vflip().data());
        auto aspect = params.aspect_ratio;
        auto angle  = params.transform.angle;
        auto anchor = params.transform.anchor;
        auto crop   = params.transform.crop;
        auto pers   = params.transform.perspective;
        pers.ur[0] -= 1.0;
        pers.lr[0] -= 1.0;
        pers.lr[1] -= 1.0;
        pers.ll[1] -= 1.0;
        std::vector<std::array<double, 2>> pers_corners = {pers.ul, pers.ur, pers.lr, pers.ll};

        auto do_crop = [&](core::frame_geometry::coord& coord) {
            if (!is_default_geometry) {
                // TODO implement support for non-default geometry.
                return;
            }

            coord.vertex_x  = std::max(coord.vertex_x, crop.ul[0]);
            coord.vertex_x  = std::min(coord.vertex_x, crop.lr[0]);
            coord.vertex_y  = std::max(coord.vertex_y, crop.ul[1]);
            coord.vertex_y  = std::min(coord.vertex_y, crop.lr[1]);
            coord.texture_x = std::max(coord.texture_x, crop.ul[0]);
            coord.texture_x = std::min(coord.texture_x, crop.lr[0]);
            coord.texture_y = std::max(coord.texture_y, crop.ul[1]);
            coord.texture_y = std::min(coord.texture_y, crop.lr[1]);
        };
        auto do_perspective = [=](core::frame_geometry::coord& coord, const std::array<double, 2>& pers_corner) {
            if (!is_default_geometry) {
                // TODO implement support for non-default geometry.
                return;
            }

            coord.vertex_x += pers_corner[0];
            coord.vertex_y += pers_corner[1];
        };
        auto rotate = [&](core::frame_geometry::coord& coord) {
            auto orig_x    = (coord.vertex_x - anchor[0]) * f_s[0];
            auto orig_y    = (coord.vertex_y - anchor[1]) * f_s[1] / aspect;
            coord.vertex_x = orig_x * std::cos(angle) - orig_y * std::sin(angle);
            coord.vertex_y = orig_x * std::sin(angle) + orig_y * std::cos(angle);
            coord.vertex_y *= aspect;
        };
        auto move = [&](core::frame_geometry::coord& coord) {
            coord.vertex_x += f_p[0];
            coord.vertex_y += f_p[1];
        };

        int corner = 0;
        for (auto& coord : coords) {
            do_crop(coord);
            do_perspective(coord, pers_corners.at(corner));
            rotate(coord);
            move(coord);

            if (++corner == 4) {
                corner = 0;
            }
        }

        // Skip drawing if all the coordinates will be outside the screen.
        if (is_outside_screen(coords)) {
            return;
        }

        // Bind textures

        for (int n = 0; n < params.textures.size(); ++n) {
            params.textures[n]->bind(n);
        }

        if (params.local_key) {
            params.local_key->bind(static_cast<int>(texture_id::local_key));
        }

        if (params.layer_key) {
            params.layer_key->bind(static_cast<int>(texture_id::layer_key));
        }

        // Setup shader

        shader_->use();

        shader_->set("plane[0]", texture_id::plane0);
        shader_->set("plane[1]", texture_id::plane1);
        shader_->set("plane[2]", texture_id::plane2);
        shader_->set("plane[3]", texture_id::plane3);
        shader_->set("local_key", texture_id::local_key);
        shader_->set("layer_key", texture_id::layer_key);
        shader_->set("is_hd", params.pix_desc.planes.at(0).height > 700 ? 1 : 0);
        shader_->set("has_local_key", static_cast<bool>(params.local_key));
        shader_->set("has_layer_key", static_cast<bool>(params.layer_key));
        shader_->set("pixel_format", params.pix_desc.format);
        shader_->set("opacity", params.transform.is_key ? 1.0 : params.transform.opacity);

        if (params.transform.chroma.enable) {
            shader_->set("chroma", true);
            shader_->set("chroma_show_mask", params.transform.chroma.show_mask);
            shader_->set("chroma_target_hue", params.transform.chroma.target_hue / 360.0);
            shader_->set("chroma_hue_width", params.transform.chroma.hue_width);
            shader_->set("chroma_min_saturation", params.transform.chroma.min_saturation);
            shader_->set("chroma_min_brightness", params.transform.chroma.min_brightness);
            shader_->set("chroma_softness", 1.0 + params.transform.chroma.softness);
            shader_->set("chroma_spill_suppress", params.transform.chroma.spill_suppress / 360.0);
            shader_->set("chroma_spill_suppress_saturation", params.transform.chroma.spill_suppress_saturation);
        } else {
            shader_->set("chroma", false);
        }

        // Setup blend_func

        if (params.transform.is_key) {
            params.blend_mode = core::blend_mode::normal;
        }

        params.background->bind(static_cast<int>(texture_id::background));
        shader_->set("background", texture_id::background);
        shader_->set("blend_mode", params.blend_mode);
        shader_->set("keyer", params.keyer);

        // Setup image-adjustements
        shader_->set("invert", params.transform.invert);

        if (params.transform.levels.min_input > epsilon || params.transform.levels.max_input < 1.0 - epsilon ||
            params.transform.levels.min_output > epsilon || params.transform.levels.max_output < 1.0 - epsilon ||
            std::abs(params.transform.levels.gamma - 1.0) > epsilon) {
            shader_->set("levels", true);
            shader_->set("min_input", params.transform.levels.min_input);
            shader_->set("max_input", params.transform.levels.max_input);
            shader_->set("min_output", params.transform.levels.min_output);
            shader_->set("max_output", params.transform.levels.max_output);
            shader_->set("gamma", params.transform.levels.gamma);
        } else {
            shader_->set("levels", false);
        }

        if (std::abs(params.transform.brightness - 1.0) > epsilon ||
            std::abs(params.transform.saturation - 1.0) > epsilon ||
            std::abs(params.transform.contrast - 1.0) > epsilon) {
            shader_->set("csb", true);

            shader_->set("brt", params.transform.brightness);
            shader_->set("sat", params.transform.saturation);
            shader_->set("con", params.transform.contrast);
        } else {
            shader_->set("csb", false);
        }

        // Setup drawing area

        GL(glViewport(0, 0, params.background->width(), params.background->height()));
        glDisable(GL_DEPTH_TEST);

        auto m_p = params.transform.clip_translation;
        auto m_s = params.transform.clip_scale;

        bool scissor = m_p[0] > std::numeric_limits<double>::epsilon() ||
                       m_p[1] > std::numeric_limits<double>::epsilon() ||
                       m_s[0] < 1.0 - std::numeric_limits<double>::epsilon() ||
                       m_s[1] < 1.0 - std::numeric_limits<double>::epsilon();

        if (scissor) {
            double w = static_cast<double>(params.background->width());
            double h = static_cast<double>(params.background->height());

            GL(glEnable(GL_SCISSOR_TEST));
            GL(glScissor(static_cast<int>(m_p[0] * w),
                         static_cast<int>(m_p[1] * h),
                         std::max(0, static_cast<int>(m_s[0] * w)),
                         std::max(0, static_cast<int>(m_s[1] * h))));
        }

        // Set render target
        params.background->attach();

        // Perspective correction
        double diagonal_intersection_x;
        double diagonal_intersection_y;

        if (get_line_intersection(pers.ul[0] + crop.ul[0],
                                  pers.ul[1] + crop.ul[1],
                                  pers.lr[0] + crop.lr[0],
                                  pers.lr[1] + crop.lr[1],
                                  pers.ur[0] + crop.lr[0],
                                  pers.ur[1] + crop.ul[1],
                                  pers.ll[0] + crop.ul[0],
                                  pers.ll[1] + crop.lr[1],
                                  diagonal_intersection_x,
                                  diagonal_intersection_y) &&
            is_default_geometry) {
            // http://www.reedbeta.com/blog/2012/05/26/quadrilateral-interpolation-part-1/
            auto d0 = hypotenuse(
                pers.ll[0] + crop.ul[0], pers.ll[1] + crop.lr[1], diagonal_intersection_x, diagonal_intersection_y);
            auto d1 = hypotenuse(
                pers.lr[0] + crop.lr[0], pers.lr[1] + crop.lr[1], diagonal_intersection_x, diagonal_intersection_y);
            auto d2 = hypotenuse(
                pers.ur[0] + crop.lr[0], pers.ur[1] + crop.ul[1], diagonal_intersection_x, diagonal_intersection_y);
            auto d3 = hypotenuse(
                pers.ul[0] + crop.ul[0], pers.ul[1] + crop.ul[1], diagonal_intersection_x, diagonal_intersection_y);

            auto ulq = calc_q(d3, d1);
            auto urq = calc_q(d2, d0);
            auto lrq = calc_q(d1, d3);
            auto llq = calc_q(d0, d2);

            std::vector<double> q_values = {ulq, urq, lrq, llq};

            corner = 0;
            for (auto& coord : coords) {
                coord.texture_q = q_values[corner];
                coord.texture_x *= q_values[corner];
                coord.texture_y *= q_values[corner];

                if (++corner == 4)
                    corner = 0;
            }
        }

        // Draw
        switch (params.geometry.type()) {
            case core::frame_geometry::geometry_type::quad: {
                GL(glBindVertexArray(vao_));
                GL(glBindBuffer(GL_ARRAY_BUFFER, vbo_));

                std::vector<core::frame_geometry::coord> coords_triangles{
                    coords[0], coords[1], coords[2], coords[0], coords[2], coords[3]};

                GL(glBufferData(GL_ARRAY_BUFFER,
                                static_cast<GLsizeiptr>(sizeof(core::frame_geometry::coord)) * coords_triangles.size(),
                                coords_triangles.data(),
                                GL_STATIC_DRAW));

                auto stride = static_cast<GLsizei>(sizeof(core::frame_geometry::coord));

                auto vtx_loc = shader_->get_attrib_location("Position");
                auto tex_loc = shader_->get_attrib_location("TexCoordIn");

                GL(glEnableVertexAttribArray(vtx_loc));
                GL(glEnableVertexAttribArray(tex_loc));

                GL(glVertexAttribPointer(vtx_loc, 2, GL_DOUBLE, GL_FALSE, stride, nullptr));
                GL(glVertexAttribPointer(tex_loc, 4, GL_DOUBLE, GL_FALSE, stride, (GLvoid*)(2 * sizeof(GLdouble))));

                GL(glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(coords_triangles.size())));
                GL(glTextureBarrier());

                GL(glDisableVertexAttribArray(vtx_loc));
                GL(glDisableVertexAttribArray(tex_loc));

                GL(glBindVertexArray(0));
                GL(glBindBuffer(GL_ARRAY_BUFFER, 0));

                break;
            }
            default:
                break;
        }

        // Cleanup
        GL(glDisable(GL_SCISSOR_TEST));
        GL(glDisable(GL_BLEND));
    }
};

image_kernel::image_kernel(const spl::shared_ptr<device>& ogl)
    : impl_(new impl(ogl))
{
}
image_kernel::~image_kernel() {}
void image_kernel::draw(const draw_params& params) { impl_->draw(params); }

}}} // namespace caspar::accelerator::ogl
