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

namespace caspar { namespace psd {

template<typename T>
class image
{
public:
	image(unsigned long width, unsigned long height, unsigned char channels) : width_(width), height_(height), channels_(channels) 
	{
		data_.resize(width*height*channels);
	}

	int width() const { return width_; };
	int height() const { return height_; };
	unsigned char channel_count() const { return channels_; }

	T* data() { return data_.data(); }

private:
	std::vector<T> data_;
	unsigned long width_;
	unsigned long height_;
	unsigned char channels_;
};

typedef image<unsigned char> image8bit; 
typedef image<unsigned short> image16bit; 
typedef image<unsigned long> image32bit; 

typedef std::shared_ptr<image8bit>  image8bit_ptr;
typedef std::shared_ptr<image16bit>  image16bit_ptr;
typedef std::shared_ptr<image32bit>  image32bit_ptr;

}	//namespace psd
}	//namespace caspar
