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

#pragma once

namespace caspar {
		
template<typename T>
struct move_on_copy
{
	move_on_copy(const move_on_copy<T>& other) : value(std::move(other.value)){}
	move_on_copy(T&& value) : value(std::move(value)){}
	mutable T value;
};

template<typename T>
move_on_copy<T> make_move_on_copy(T&& value)
{
	return move_on_copy<T>(std::move(value));
}

}