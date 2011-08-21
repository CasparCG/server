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
#include "../../stdafx.h"

#include "audio_transform.h"

namespace caspar { namespace core {
	
audio_transform::audio_transform()
	: volume_(1.0)
	, has_audio_(true){}

void audio_transform::set_volume(double value)
{
	volume_ = std::max(0.0, value);
}

double audio_transform::get_volume() const
{
	return volume_;
}

void audio_transform::set_has_audio(bool value)
{
	has_audio_ = value;
}

bool audio_transform::get_has_audio() const
{
	return has_audio_;
}

audio_transform& audio_transform::operator*=(const audio_transform &other) 
{
	volume_ *= other.volume_;
	has_audio_ &= other.has_audio_;
	return *this;
}

const audio_transform audio_transform::operator*(const audio_transform &other) const
{
	return audio_transform(*this) *= other;
}

audio_transform tween(double time, const audio_transform& source, const audio_transform& dest, double duration, const tweener_t& tweener)
{
	auto do_tween = [](double time, double source, double dest, double duration, const tweener_t& tweener)
	{
		return tweener(time, source, dest-source, duration);
	};

	audio_transform result;
	result.set_volume(do_tween(time, source.get_volume(), dest.get_volume(), duration, tweener));
	result.set_has_audio(source.get_has_audio() || dest.get_has_audio());
	return result;
}

}}