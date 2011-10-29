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

#include <common/memory/safe_ptr.h>

#include <boost/noncopyable.hpp>

#include <algorithm>
#include <functional>
#include <string>
#include <vector>
#include <stdint.h>
#include <numeric>

namespace caspar { namespace core {

class basic_frame;
struct frame_factory;

struct frame_producer : boost::noncopyable
{
public:
	enum hints
	{
		NO_HINT = 0,
		ALPHA_HINT = 1
	};

	virtual ~frame_producer(){}	

	virtual std::wstring print() const = 0; // nothrow

	virtual bool param(const std::wstring&){return false;}

	virtual safe_ptr<frame_producer> get_following_producer() const {return frame_producer::empty();}  // nothrow
	virtual void set_leading_producer(const safe_ptr<frame_producer>&) {}  // nothrow
		
	virtual int64_t nb_frames() const {return std::numeric_limits<int>::max();}
	
	virtual safe_ptr<basic_frame> receive(int hints) = 0;
	virtual safe_ptr<core::basic_frame> last_frame() const = 0;

	static const safe_ptr<frame_producer>& empty(); // nothrow
};

safe_ptr<basic_frame> receive_and_follow(safe_ptr<frame_producer>& producer, int hints);

typedef std::function<safe_ptr<core::frame_producer>(const safe_ptr<frame_factory>&, const std::vector<std::wstring>&)> producer_factory_t;
void register_producer_factory(const producer_factory_t& factory); // Not thread-safe.
safe_ptr<core::frame_producer> create_producer(const safe_ptr<frame_factory>&, const std::vector<std::wstring>& params);

template<typename T>
typename std::decay<T>::type get_param(const std::wstring& name, const std::vector<std::wstring>& params, T fail_value)
{	
	auto it = std::find(params.begin(), params.end(), name);
	if(it == params.end() || ++it == params.end())	
		return fail_value;
	
	T value = fail_value;
	try
	{
		value = boost::lexical_cast<std::decay<T>::type>(*it);
	}
	catch(boost::bad_lexical_cast&){}

	return value;
}

}}
