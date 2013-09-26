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
* Author: Niklas P Andersson, niklas.p.andersson@svt.se
*/

#pragma once

#include <memory>
#include <vector>

namespace caspar { namespace core {

class frame_geometry
{
public:
	enum geometry_type
	{
		none,
		quad,
		quad_list
	};

	frame_geometry();
	frame_geometry(geometry_type, std::vector<float>&&);

	geometry_type type() const ;
	const std::vector<float>& data() const;

	static const frame_geometry& get_default();

private:
	struct impl;
	std::shared_ptr<impl> impl_;
};

}}