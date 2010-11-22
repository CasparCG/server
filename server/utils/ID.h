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
 
#ifndef _ID_H_
#define _ID_H_

namespace caspar
{
	namespace utils
	{
		class ID 
		{ 			
		public:
			typedef long long value_type;

			ID();
			const value_type& Value() const;
			static ID Generate(void* ptr);
		private:
			value_type value_; 
		};

		class Identifiable
		{
		public:
			Identifiable();
			virtual ~Identifiable(){}
			const utils::ID& ID() const;
		private:
			const utils::ID id_;
		};

		bool operator==(const ID& lhs,const ID& rhs);
		bool operator!=(const ID& lhs,const ID& rhs);
	}
}

#endif