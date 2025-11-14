#pragma once

#include <boost/numeric/ublas/matrix.hpp>
#include <boost/numeric/ublas/matrix_vector.hpp>

#include <core/frame/geometry.h>

namespace caspar::accelerator::vulkan {

typedef boost::numeric::ublas::matrix<double, boost::numeric::ublas::row_major, std::vector<double>> t_matrix;

typedef boost::numeric::ublas::vector<double, std::vector<double>> t_point;

t_matrix get_vertex_matrix(const core::image_transform& transform, double aspect_ratio);

} // namespace caspar::accelerator::vulkan

namespace boost::numeric::ublas {
template <typename T, typename L, typename S>
boost::numeric::ublas::matrix<T, L, S> operator*(const boost::numeric::ublas::matrix<T, L, S>& lhs,
                                                 const boost::numeric::ublas::matrix<T, L, S>& rhs)
{
    return boost::numeric::ublas::matrix<T>(boost::numeric::ublas::prod(lhs, rhs));
}
template <typename T, typename L, typename S1, typename S2>
boost::numeric::ublas::vector<T, S1> operator*(const boost::numeric::ublas::vector<T, S1>&    lhs,
                                               const boost::numeric::ublas::matrix<T, L, S2>& rhs)
{
    return boost::numeric::ublas::vector<T, S1>(boost::numeric::ublas::prod(lhs, rhs));
}
template <typename T, typename L, typename S1, typename S2>
bool operator==(const boost::numeric::ublas::matrix<T, L, S1>& lhs, const boost::numeric::ublas::matrix<T, L, S2>& rhs)
{
    if (lhs.size1() != rhs.size1() || lhs.size2() != rhs.size2())
        return false;
    for (int y = 0; y < lhs.size1(); ++y)
        for (int x = 0; x < lhs.size2(); ++x)
            if (lhs(y, x) != rhs(y, x))
                return false;
    return true;
}
} // namespace boost::numeric::ublas