

#include <core/frame/frame_transform.h>

#include <common/except.h>
#include <common/log.h>

#include <cmath>
#include <utility>
#include <vector>

#include <boost/numeric/ublas/io.hpp>
#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_vector.hpp>
#include <boost/numeric/ublas/vector.hpp>

#include "matrix.h"

namespace caspar::accelerator::ogl {

t_matrix create_matrix(std::vector<std::vector<double>> data)
{
    if (data.empty())
        CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(L"data cannot be empty"));

    t_matrix matrix(data.size(), data.at(0).size());
    for (int y = 0; y < matrix.size1(); ++y) {
        if (data.at(y).size() != matrix.size2())
            CASPAR_THROW_EXCEPTION(invalid_argument() << msg_info(L"Each row must be of the same length"));

        for (int x = 0; x < matrix.size2(); ++x)
            matrix(x, y) = data.at(y).at(x);
    }
    return matrix;
}

t_matrix get_vertex_matrix(const core::image_transform& transform, double aspect_ratio)
{
    using namespace boost::numeric::ublas;
    auto anchor_matrix =
        create_matrix({{1.0, 0.0, -transform.anchor[0]}, {0.0, 1.0, -transform.anchor[1]}, {0.0, 0.0, 1.0}});
    auto scale_matrix =
        create_matrix({{transform.fill_scale[0], 0.0, 0.0}, {0.0, transform.fill_scale[1], 0.0}, {0.0, 0.0, 1.0}});
    auto aspect_matrix      = create_matrix({{1.0, 0.0, 0.0}, {0.0, 1.0 / aspect_ratio, 0.0}, {0.0, 0.0, 1.0}});
    auto aspect_inv_matrix  = create_matrix({{1.0, 0.0, 0.0}, {0.0, aspect_ratio, 0.0}, {0.0, 0.0, 1.0}});
    auto rotation_matrix    = create_matrix({{std::cos(transform.angle), -std::sin(transform.angle), 0.0},
                                             {std::sin(transform.angle), std::cos(transform.angle), 0.0},
                                             {0.0, 0.0, 1.0}});
    auto translation_matrix = create_matrix(
        {{1.0, 0.0, transform.fill_translation[0]}, {0.0, 1.0, transform.fill_translation[1]}, {0.0, 0.0, 1.0}});

    return anchor_matrix * aspect_matrix * scale_matrix * rotation_matrix * aspect_inv_matrix * translation_matrix;
}

} // namespace caspar::accelerator::ogl
