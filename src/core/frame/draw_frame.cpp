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
#include "draw_frame.h"

#include "frame.h"
#include "frame_transform.h"
#include "frame_visitor.h"

#include <boost/variant.hpp>

namespace caspar { namespace core {

using frame_t = boost::variant<boost::blank, const_frame, std::vector<draw_frame>>;

struct draw_frame::impl
{
    frame_t         frame_;
    frame_transform transform_;

  public:
    impl() {}

    impl(frame_t frame)
        : frame_(std::move(frame))
    {
    }

    void accept(frame_visitor& visitor) const
    {
        struct accept_visitor : public boost::static_visitor<void>
        {
            frame_visitor& visitor;

            accept_visitor(frame_visitor& visitor)
                : visitor(visitor)
            {
            }

            void operator()(boost::blank /*unused*/) const {}

            void operator()(const const_frame& frame) const { visitor.visit(frame); }

            void operator()(const std::vector<draw_frame>& frames) const
            {
                for (auto& frame : frames) {
                    frame.accept(visitor);
                }
            }
        };
        visitor.push(transform_);
        boost::apply_visitor(accept_visitor{visitor}, frame_);
        visitor.pop();
    }

    bool operator==(const impl& other) { return frame_ == other.frame_ && transform_ == other.transform_; }
};

draw_frame::draw_frame()
    : impl_(new impl())
{
}
draw_frame::draw_frame(const draw_frame& other)
    : impl_(new impl())
{
    impl_->frame_     = other.impl_->frame_;
    impl_->transform_ = other.impl_->transform_;
}

draw_frame::draw_frame(draw_frame&& other)
    : impl_(std::move(other.impl_))
{
}
draw_frame::draw_frame(const_frame frame)
    : impl_(new impl(std::move(frame)))
{
}
draw_frame::draw_frame(mutable_frame&& frame)
    : impl_(new impl(std::move(frame)))
{
}
draw_frame::draw_frame(std::vector<draw_frame> frames)
    : impl_(new impl(std::move(frames)))
{
}
draw_frame::~draw_frame() {}
draw_frame& draw_frame::operator=(draw_frame other)
{
    impl_ = std::move(other.impl_);
    return *this;
}
void                   draw_frame::swap(draw_frame& other) { impl_.swap(other.impl_); }
const frame_transform& draw_frame::transform() const { return impl_->transform_; }
frame_transform&       draw_frame::transform() { return impl_->transform_; }
void                   draw_frame::accept(frame_visitor& visitor) const { impl_->accept(visitor); }
bool draw_frame::operator==(const draw_frame& other) const { return impl_ && *impl_ == *other.impl_; }
bool draw_frame::operator!=(const draw_frame& other) const { return !(*this == other); }

draw_frame draw_frame::over(draw_frame frame1, draw_frame frame2)
{
    if (!frame1 && !frame2) {
        return draw_frame{};
    }

    std::vector<draw_frame> frames;
    frames.push_back(std::move(frame1));
    frames.push_back(std::move(frame2));
    return draw_frame(std::move(frames));
}

draw_frame draw_frame::mask(draw_frame fill, draw_frame key)
{
    if (!fill || !key) {
        return draw_frame{};
    }

    std::vector<draw_frame> frames;
    key.transform().image_transform.is_key = true;
    frames.push_back(std::move(key));
    frames.push_back(std::move(fill));
    return draw_frame(std::move(frames));
}

draw_frame draw_frame::push(draw_frame frame)
{
    std::vector<draw_frame> frames;
    frames.push_back(std::move(frame));
    return draw_frame(std::move(frames));
}

draw_frame draw_frame::push(draw_frame frame, const frame_transform& transform)
{
    std::vector<draw_frame> frames;
    frames.push_back(std::move(frame));
    auto result        = draw_frame(std::move(frames));
    result.transform() = std::move(transform);
    return result;
}

draw_frame draw_frame::pop(const draw_frame& frame)
{
    draw_frame result;
    result.impl_->frame_ = frame.impl_->frame_;
    return result;
}

draw_frame draw_frame::still(draw_frame frame)
{
    frame.transform().audio_transform.volume = 0.0;
    return frame;
}

draw_frame draw_frame::empty() { return draw_frame(std::vector<draw_frame>{}); }

draw_frame::operator bool() const { return impl_ && impl_->frame_.which() != 0; }

}} // namespace caspar::core
