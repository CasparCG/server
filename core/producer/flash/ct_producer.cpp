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

#include "ct_producer.h"

#include "cg_producer.h"

namespace caspar { namespace core { namespace flash {

struct ct_producer : public cg_producer
{
	ct_producer(ct_producer&& other) : initialized_(std::move(other.initialized_)), filename_(std::move(other.filename_)){}
	ct_producer(const std::wstring& filename) : filename_(filename), initialized_(false){}

	virtual safe_ptr<draw_frame> receive()
	{
		if(!initialized_)
		{
			cg_producer::add(0, filename_, 1);
			initialized_ = true;
		}
		return cg_producer::receive();
	}

	virtual std::wstring print() const
	{
		return L"ct[" + filename_ + L"]";
	}

	bool initialized_;
	std::wstring filename_;
};
	
safe_ptr<frame_producer> create_ct_producer(const std::vector<std::wstring>& params) 
{
	std::wstring filename = params[0] + L".ct";
	return boost::filesystem::exists(filename) ? make_safe<ct_producer>(filename) : frame_producer::empty();
}

}}}