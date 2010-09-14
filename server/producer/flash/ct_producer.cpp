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

#include "../../../common/utility/find_file.h"
#include "../../frame/frame.h"
#include "../../server.h"

#include <boost/assign/list_of.hpp>

using namespace boost::assign;

namespace caspar { namespace flash {
	
frame_producer_ptr create_ct_producer(const std::vector<std::wstring>& params, const frame_format_desc& format_desc) 
{
	std::wstring filename = params[0];
	std::wstring result_filename = common::find_file(server::media_folder() + filename, list_of(L"ct"));
	if(result_filename.empty())
		return nullptr;
	
	std::wstring fixed_filename = result_filename;
	std::wstring::size_type pos = 0;
	while((pos = fixed_filename.find(TEXT('\\'), pos)) != std::wstring::npos) 
		fixed_filename[pos] = TEXT('/');
	
	cg_producer_ptr cg_producer(new cg_producer(format_desc));
	cg_producer->add(0, filename, 1);
	return cg_producer;
}

}}