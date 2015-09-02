/*
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
#include <cstdint>

namespace caspar { namespace psd {

template<typename T>
class image
{
public:
	image(std::uint32_t width, std::uint32_t height, std::uint8_t channels) : width_(width), height_(height), channels_(channels)
	{
		data_.resize(width*height*channels);
	}

	std::uint32_t width() const { return width_; }
	std::uint32_t height() const { return height_; }
	std::uint8_t channel_count() const { return channels_; }

	T* data() { return data_.data(); }

private:
	std::vector<T> data_;
	std::uint32_t width_;
	std::uint32_t height_;
	std::uint8_t channels_;
};

typedef image<std::uint8_t>		image8bit;
typedef image<std::uint16_t>	image16bit;
typedef image<std::uint32_t>	image32bit;

typedef std::shared_ptr<image8bit>	image8bit_ptr;
typedef std::shared_ptr<image16bit>	image16bit_ptr;
typedef std::shared_ptr<image32bit>	image32bit_ptr;

}	//namespace psd
}	//namespace caspar
