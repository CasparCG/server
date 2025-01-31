#pragma once

#include <core/mixer/image/blend_modes.h>

#include <common/memory.h>

#include <core/frame/frame_transform.h>
#include <core/frame/geometry.h>
#include <core/frame/pixel_format.h>

#include <utility>

#include "matrix.h"

namespace caspar::accelerator::ogl {

struct draw_crop_region
{
    explicit draw_crop_region(double left, double top, double right, double bottom);

    void apply_transform(const t_matrix& matrix);

    std::array<t_point, 4> coords;
};

struct draw_transform_step
{
    draw_transform_step()
        : vertex_matrix(boost::numeric::ublas::identity_matrix<double>(3, 3))
    {
    }

    draw_transform_step(const core::corners& perspective, const t_matrix& vertex_matrix)
        : perspective(perspective)
        , vertex_matrix(vertex_matrix)
    {
    }

    core::corners perspective;

    std::vector<draw_crop_region> crop_regions;

    t_matrix vertex_matrix;
};

struct draw_transforms
{
    std::vector<draw_transform_step> steps;

    draw_transforms()
        : image_transform(core::image_transform())
        , steps({draw_transform_step()})
    {
    }

    explicit draw_transforms(core::image_transform transform, std::vector<draw_transform_step> steps)
        : image_transform(transform)
        , steps(std::move(steps))
    {
    }

    core::image_transform image_transform;

    draw_transform_step& current() { return steps.back(); }

    [[nodiscard]] draw_transforms combine_transform(const core::image_transform& transform, double aspect_ratio) const;

    [[nodiscard]] std::vector<core::frame_geometry::coord> transform_coords(const std::vector<core::frame_geometry::coord>& coords) const;
};

} // namespace caspar::accelerator::ogl