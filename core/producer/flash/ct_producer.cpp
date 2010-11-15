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

#include "../../processor/frame.h"
#include "../../server.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

namespace caspar { namespace core { namespace flash {
	
frame_producer_ptr create_ct_producer(const std::vector<std::wstring>& params) 
{
	static const std::vector<std::wstring> extensions = list_of(L"ct");
	std::wstring filename = server::media_folder() + L"\\" + params[0];
	
	auto ext = std::find_if(extensions.begin(), extensions.end(), [&](const std::wstring& ex) -> bool
		{					
			return boost::filesystem::is_regular_file(boost::filesystem::wpath(filename).replace_extension(ex));
		});

	if(ext == extensions.end())
		return nullptr;
	
	auto producer = std::make_shared<cg_producer>();
	producer->add(0, filename, 1);
	return producer;
}

}

}}	//namespace caspar