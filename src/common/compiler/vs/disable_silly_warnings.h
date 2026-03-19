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

#pragma warning(disable : 4100) // unreferenced formal parameter
#pragma warning(disable : 4127) // conditional expression is constant
#pragma warning(disable : 4180) // qualifier applied to function type has no meaning; ignored
#pragma warning(disable : 4244) // conversion from 'type1' to 'type2', possible loss of data
#pragma warning(disable : 4245) // conversion from 'type1' to 'type2', signed/unsigned mismatch
#pragma warning(disable : 4324) // padding a structure for alignment
#pragma warning(disable : 4334) // result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
#pragma warning(disable : 4355) // 'this' : used in base member initializer list
#pragma warning(disable : 4459) // declaration of 'query' hides global declaration
#pragma warning(disable : 4481) // nonstandard extension used: override specifier 'override'
#pragma warning(disable : 4482) // nonstandard extension used: enum 'enum' used in qualified name
#pragma warning(disable : 4503) // decorated name length exceeded, name was truncated
#pragma warning(disable : 4505) // unreferenced local function has been
#pragma warning(disable : 4512) // assignment operator could not be generated
#if (_MSC_VER > 1800 && _MSC_FULL_VER >= 190023506)
#pragma warning(disable : 4592) // symbol will be dynamically initialized (implementation limitation)
#endif
#pragma warning(disable : 4702) // unreachable code
#pragma warning(disable : 4714) // marked as __forceinline not inlined
#pragma warning(disable : 4834) // discarding return value of function with 'nodiscard' attribute
#pragma warning(disable : 4996) // function call with parameters that may be unsafe
