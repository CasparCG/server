/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
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

#include "util/bigendian_file_input_stream.h"

#include <boost/property_tree/ptree.hpp>

#include <vector>
#include <map>
#include <string>
#include <memory>

namespace caspar { namespace psd {

class descriptor
{
	typedef boost::property_tree::wptree Ptree;
	struct context
	{
		typedef std::shared_ptr<context> ptr_type;

		Ptree root;
		std::vector<Ptree *> stack;
		std::wstring debug_name;

		friend descriptor;
		friend class scoped_holder;
		class scoped_holder;

		context(const std::wstring& debug_name)
			: debug_name(debug_name)
		{
		}
	};
	friend class context::scoped_holder;

	descriptor(const descriptor&);
	const descriptor& operator=(const descriptor&);
	explicit descriptor(const std::wstring& key, context::ptr_type context);

public:
	descriptor(const std::wstring& debug_name);
	~descriptor();

	void populate(bigendian_file_input_stream& stream);
	Ptree& items() const { return context_->root; }
private:
	void read_value(const std::wstring& key, bigendian_file_input_stream& stream);
	context::ptr_type context_;
};

}	//namespace psd
}	//namespace caspar
