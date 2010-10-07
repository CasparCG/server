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
 
#include "stdafx.h"
#include "ID.h"

namespace caspar
{
	namespace utils
	{

		ID::ID() : value_(0){}

		const ID::value_type& ID::Value() const
		{
			return value_;
		}
		ID ID::Generate(void* ptr)
		{			
			assert(sizeof(value_type) >= sizeof(LARGE_INTEGER));
			ID id;
			QueryPerformanceCounter(reinterpret_cast<LARGE_INTEGER*>(&id.value_));	
			id.value_ <<= 32;
			id.value_ |= reinterpret_cast<int>(ptr);
			return id;
		}

		bool operator ==(const ID& lhs,const ID& rhs)
		{
			return lhs.Value() == rhs.Value();
		}

		Identifiable::Identifiable() : id_(utils::ID::Generate(this))
		{}

		const utils::ID& Identifiable::ID() const
		{
			return id_;
		}
	}
}