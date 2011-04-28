/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#pragma once

#include <common/utility/tweener.h>

namespace caspar { namespace core {

class audio_transform
{
public:
	audio_transform();

	void set_gain(double value);
	double get_gain() const;
	
	void set_has_audio(bool value);
	bool get_has_audio() const;

	audio_transform& operator*=(const audio_transform &other);
	const audio_transform operator*(const audio_transform &other) const;
private:
	double gain_;
	bool has_audio_;
};

audio_transform tween(double time, const audio_transform& source, const audio_transform& dest, double duration, const tweener_t& tweener);

inline bool operator==(const audio_transform& lhs, const audio_transform& rhs)
{
	return memcmp(&lhs, &rhs, sizeof(audio_transform)) == 0;
}

inline bool operator!=(const audio_transform& lhs, const audio_transform& rhs)
{
	return !(lhs == rhs);
}

}}