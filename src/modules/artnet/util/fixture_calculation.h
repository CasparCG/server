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

namespace caspar { namespace artnet {

enum FixtureType
{
    DIMMER = 1,
    RGB    = 3,
    RGBW   = 4,
};

struct box
{
    // top-left corner
    float x;
    float y;

    float width;
    float height;
};

struct fixture_flux
{
    // Relative luminous output per unit DMX value, per LED. Used to compensate when LEDs of
    // different colors don't share the same brightness curve (typical of cheap RGBW strips).
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
    float w = 1.0f;
};

struct fixture
{
    FixtureType    type;
    unsigned short startAddress;    // DMX address of the first channel in the fixture
    unsigned short fixtureCols;     // columns in the fixture grid (dividing along the width)
    unsigned short fixtureRows;     // rows in the fixture grid (dividing along the height)
    unsigned short fixtureChannels; // number of channels per fixture

    fixture_flux flux;
    box          fixtureBox;
};

}} // namespace caspar::artnet
