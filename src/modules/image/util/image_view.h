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
 * Author: Helge Norberg, helge.norberg@svt.se
 */

#pragma once

#include <boost/iterator/filter_iterator.hpp>

namespace caspar { namespace image {

/**
 * A POD pixel with a compatible memory layout as a 8bit BGRA pixel (32bits in
 * total).
 * <p>
 * Models the PackedPixel concept used by for example image_view. Also models
 * the RGBAPixel concept which does not care about the order between RGBA but
 * only requires that all 4 channel has accessors.
 */
class bgra_pixel
{
    uint8_t b_;
    uint8_t g_;
    uint8_t r_;
    uint8_t a_;

  public:
    bgra_pixel(uint8_t b = 0, uint8_t g = 0, uint8_t r = 0, uint8_t a = 0)
        : b_(b)
        , g_(g)
        , r_(r)
        , a_(a)
    {
    }

    const uint8_t& b() const { return b_; }
    uint8_t&       b() { return b_; }
    const uint8_t& g() const { return g_; }
    uint8_t&       g() { return g_; }
    const uint8_t& r() const { return r_; }
    uint8_t&       r() { return r_; }
    const uint8_t& a() const { return a_; }
    uint8_t&       a() { return a_; }
};

template <class PackedPixel>
class image_sub_view;

/**
 * An image view abstracting raw packed pixel data
 * <p>
 * This is only a view, it does not own the data.
 * <p>
 * Models the the ImageView concept.
 */
template <class PackedPixel>
class image_view
{
  public:
    using pixel_type = PackedPixel;

    image_view(void* raw_start, int width, int height)
        : begin_(static_cast<PackedPixel*>(raw_start))
        , end_(begin_ + width * height)
        , width_(width)
        , height_(height)
    {
    }

    PackedPixel* begin() { return begin_; }

    const PackedPixel* begin() const { return begin_; }

    PackedPixel* end() { return end_; }

    const PackedPixel* end() const { return end_; }

    template <class PackedPixelIter>
    PackedPixel* relative(PackedPixelIter to, int delta_x, int delta_y)
    {
        auto         pixel_distance = delta_x + width_ * delta_y;
        PackedPixel* to_address     = &(*to);
        auto         result         = to_address + pixel_distance;

        if (result < begin_ || result >= end_)
            return nullptr;
        return result;
    }

    template <class PackedPixelIter>
    const PackedPixel* relative(PackedPixelIter to, int delta_x, int delta_y) const
    {
        auto               pixel_distance = delta_x + width_ * delta_y;
        const PackedPixel* to_address     = &(*to);
        auto               result         = to_address + pixel_distance;

        if (result < begin_ || result >= end_)
            return nullptr;
        return result;
    }

    int width() const { return width_; }

    int height() const { return height_; }

    image_sub_view<PackedPixel> subview(int x, int y, int width, int height)
    {
        return image_sub_view<PackedPixel>(*this, x, y, width, height);
    }

    const image_sub_view<PackedPixel> subview(int x, int y, int width, int height) const
    {
        return image_sub_view<PackedPixel>(*this, x, y, width, height);
    }

  private:
    PackedPixel* begin_;
    PackedPixel* end_;
    int          width_;
    int          height_;
};

template <class PackedPixel>
class is_within_view
{
  public:
    is_within_view(const PackedPixel* begin, int width, int stride)
        : begin_(begin)
        , width_(width)
        , stride_(stride)
        , no_check_(width == stride)
    {
    }

    bool operator()(const PackedPixel& pixel) const
    {
        if (no_check_)
            return true;

        const PackedPixel* position                = &pixel;
        int                distance_from_row_start = (position - begin_) % stride_;

        return distance_from_row_start < width_;
    }

  private:
    const PackedPixel* begin_;
    int                width_;
    int                stride_;
    bool               no_check_;
};

template <class PackedPixel>
struct image_stride_iterator : public boost::filter_iterator<is_within_view<PackedPixel>, PackedPixel*>
{
    image_stride_iterator(PackedPixel* begin, PackedPixel* end, int width, int stride)
        : boost::filter_iterator<is_within_view<PackedPixel>, PackedPixel*>::filter_iterator(
              is_within_view<PackedPixel>(begin, width, stride),
              begin,
              end)
    {
    }
};

/**
 * A sub view created from an image_view.
 * <p>
 * This also models the ImageView concept.
 */
template <class PackedPixel>
class image_sub_view
{
  private:
    image_view<PackedPixel> root_view_;
    int                     relative_to_root_x_;
    int                     relative_to_root_y_;
    int                     width_;
    int                     height_;
    PackedPixel* raw_begin_ = root_view_.relative(root_view_.begin(), relative_to_root_x_, relative_to_root_y_);
    PackedPixel* raw_end_   = root_view_.relative(raw_begin_, width_ - 1, height_ - 1) + 1;

  public:
    using pixel_type = PackedPixel;

    image_sub_view(image_view<PackedPixel>& root_view, int x, int y, int width, int height)
        : root_view_(root_view)
        , relative_to_root_x_(x)
        , relative_to_root_y_(y)
        , width_(width)
        , height_(height)
    {
    }

    image_stride_iterator<PackedPixel> begin()
    {
        return image_stride_iterator<PackedPixel>(raw_begin_, raw_end_, width_, root_view_.width());
    }

    image_stride_iterator<const PackedPixel> begin() const
    {
        return image_stride_iterator<const PackedPixel>(raw_begin_, raw_end_, width_, root_view_.width());
    }

    image_stride_iterator<PackedPixel> end()
    {
        return image_stride_iterator<PackedPixel>(raw_end_, raw_end_, width_, root_view_.width());
    }

    image_stride_iterator<const PackedPixel> end() const
    {
        return image_stride_iterator<const PackedPixel>(raw_end_, raw_end_, width_, root_view_.width());
    }

    template <class PackedPixelIter>
    PackedPixel* relative(PackedPixelIter to, int delta_x, int delta_y)
    {
        return root_view_.relative(to, delta_x, delta_y);
    }

    template <class PackedPixelIter>
    const PackedPixel* relative(PackedPixelIter to, int delta_x, int delta_y) const
    {
        return root_view_.relative(to, delta_x, delta_y);
    }

    int width() const { return width_; }

    int height() const { return height_; }

    image_sub_view<PackedPixel> subview(int x, int y, int width, int height)
    {
        return root_view_.subview(relative_to_root_x_ + x, relative_to_root_y_ + y, width, height);
    }

    const image_sub_view<PackedPixel> subview(int x, int y, int width, int height) const
    {
        return root_view_.subview(relative_to_root_x_ + x, relative_to_root_y_ + y, width, height);
    }
};

}} // namespace caspar::image
