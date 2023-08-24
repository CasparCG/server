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
 * Author: Eliyah Sundström eliyah@sundstroem.com
 */

#pragma once

#include <core/frame/frame.h>

#include <cmath>
#include <utility>

namespace caspar { namespace artnet {

enum FixtureType
{
    DIMMER = 1,
    RGB    = 3,
    RGBW   = 4,
};

struct point
{
    float x;
    float y;
};

struct rect
{
    point p1;
    point p2;
    point p3;
    point p4;
};

struct computed_fixture
{
    FixtureType    type;
    unsigned short address;

    rect rectangle;
};

struct color
{
    std::uint8_t r;
    std::uint8_t g;
    std::uint8_t b;
};

struct box
{
    float x;
    float y;

    float width;
    float height;

    float rotation; // degrees
};

struct fixture
{
    FixtureType    type;
    unsigned short startAddress;    // DMX address of the first channel in the fixture
    unsigned short fixtureCount;    // number of fixtures in the chain, dividing along the width
    unsigned short fixtureChannels; // number of channels per fixture

    box fixtureBox;
};

rect  compute_rect(box fixtureBox, int index, int count);
color average_color(const core::const_frame& frame, rect& rectangle);

}} // namespace caspar::artnet