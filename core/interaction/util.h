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

#include <map>

#include "../frame/frame_transform.h"

namespace caspar { namespace core {

template<typename Derived, typename Base>
bool is(const spl::shared_ptr<Base>& base)
{
	return dynamic_cast<const Derived*>(base.get()) != nullptr;
}

template<typename Derived, typename Base>
spl::shared_ptr<const Derived> as(const spl::shared_ptr<Base>& base)
{
	return spl::dynamic_pointer_cast<const Derived>(base);
}

static std::pair<double, double> translate(
		double x, double y, const frame_transform& transform)
{
	auto& img_transform = transform.image_transform;
	auto fill_x = img_transform.fill_translation[0];
	auto fill_y = img_transform.fill_translation[1];
	auto scale_x = img_transform.fill_scale[0];
	auto scale_y = img_transform.fill_scale[1];

	if (fill_x != 0.0 || fill_y != 0.0 || scale_x != 1.0 || scale_y != 1.0)
	{
		double translated_x = (x - fill_x) / scale_x;
		double translated_y = (y - fill_y) / scale_y;

		return std::make_pair(translated_x, translated_y);
	}
	else
	{
		return std::make_pair(x, y);
	}
}

}}
