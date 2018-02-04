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

#include <common/memory.h>

#include "../frame/frame_transform.h"

#include "util.h"

namespace caspar { namespace core {

struct interaction_event : public spl::enable_shared_from_this<interaction_event>
{
    typedef spl::shared_ptr<const interaction_event> ptr;

    const int source_id;

    interaction_event(int source_id)
        : source_id(source_id)
    {
    }

    virtual ~interaction_event() {}
};

struct position_event : public interaction_event
{
    const double x;
    const double y;

    position_event(int source_id, double x, double y)
        : interaction_event(source_id)
        , x(x)
        , y(y)
    {
    }

    virtual interaction_event::ptr translate(const frame_transform& transform) const
    {
        auto translated = ::caspar::core::translate(x, y, transform);

        if (translated.first == x && translated.second == y)
            return shared_from_this();
        else
            return clone(translated.first, translated.second);
    }

  protected:
    virtual interaction_event::ptr clone(double new_x, double new_y) const = 0;
};

struct mouse_move_event : public position_event
{
    typedef spl::shared_ptr<mouse_move_event> ptr;

    mouse_move_event(int source_id, double x, double y)
        : position_event(source_id, x, y)
    {
    }

  protected:
    virtual interaction_event::ptr clone(double new_x, double new_y) const override { return spl::make_shared<mouse_move_event>(source_id, new_x, new_y); }
};

struct mouse_wheel_event : public position_event
{
    typedef spl::shared_ptr<mouse_wheel_event> ptr;

    const int ticks_delta;

    mouse_wheel_event(int source_id, double x, double y, int ticks_delta)
        : position_event(source_id, x, y)
        , ticks_delta(ticks_delta)
    {
    }

  protected:
    virtual interaction_event::ptr clone(double new_x, double new_y) const override
    {
        return spl::make_shared<mouse_wheel_event>(source_id, new_x, new_y, ticks_delta);
    }
};

struct mouse_button_event : public position_event
{
    typedef spl::shared_ptr<mouse_button_event> ptr;

    const int  button;
    const bool pressed;

    mouse_button_event(int source_id, double x, double y, int button, bool pressed)
        : position_event(source_id, x, y)
        , button(button)
        , pressed(pressed)
    {
    }

  protected:
    virtual interaction_event::ptr clone(double new_x, double new_y) const override
    {
        return spl::make_shared<mouse_button_event>(source_id, new_x, new_y, button, pressed);
    }
};

}} // namespace caspar::core
