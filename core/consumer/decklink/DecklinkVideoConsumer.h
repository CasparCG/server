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

#include "../../consumer/frame_consumer.h"

#include "../../frame/frame_fwd.h"

namespace caspar { namespace decklink {

class DecklinkVideoConsumer : public frame_consumer
{
public:
	explicit DecklinkVideoConsumer(const caspar::frame_format_desc& format_desc, bool internalKey = false);
	
	void display(const frame_ptr&);
	const frame_format_desc& get_frame_format_desc() const;
private:
	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};

}}