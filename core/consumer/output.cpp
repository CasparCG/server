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
// TODO: Try to recover consumer from bad_alloc...
#include "../StdAfx.h"

#ifdef _MSC_VER
#pragma warning (disable : 4244)
#endif

#include "output.h"

#include "../video_format.h"
#include "../mixer/gpu/ogl_device.h"
#include "../mixer/read_frame.h"

#include <common/utility/assert.h>
#include <common/utility/timer.h>

#include <concrt_extras.h>

using namespace Concurrency;

namespace caspar { namespace core {
	
struct output::implementation
{	
	typedef std::pair<safe_ptr<read_frame>, safe_ptr<read_frame>> fill_and_key;
	
	const video_format_desc format_desc_;

	std::map<int, safe_ptr<frame_consumer>> consumers_;
	typedef std::map<int, safe_ptr<frame_consumer>>::value_type layer_t;
	
	high_prec_timer timer_;

	critical_section				mutex_;
	call<output::source_element_t>	output_;
		
public:
	implementation(output::source_t& source, const video_format_desc& format_desc) 
		: format_desc_(format_desc)
		, output_(std::bind(&implementation::execute, this, std::placeholders::_1))
	{
		source.link_target(&output_);
	}	
	
	void add(int index, safe_ptr<frame_consumer>&& consumer)
	{		
		// C-TODO: Consumers are destroyed asynchronously, need to wait for destruction to finish before callign initialize.
		remove(index);

		consumer->initialize(format_desc_);		
		
		{
			critical_section::scoped_lock lock(mutex_);

			consumers_.insert(std::make_pair(index, consumer));

			CASPAR_LOG(info) << print() << L" " << consumer->print() << L" Added.";	
		}
	}

	void remove(int index)
	{
		{
			critical_section::scoped_lock lock(mutex_);

			auto it = consumers_.find(index);
			if(it != consumers_.end())
			{
				CASPAR_LOG(info) << print() << L" " << it->second->print() << L" Removed.";
				consumers_.erase(it);
			}
		}
	}
						
	void execute(const output::source_element_t& element)
	{	
		auto frame = element.first;

		{
			critical_section::scoped_lock lock(mutex_);		

			if(!has_synchronization_clock() || frame->image_size() != format_desc_.size)
			{		
				scoped_oversubcription_token oversubscribe;
				timer_.tick(1.0/format_desc_.fps);
			}
	
			std::vector<int> removables;		
			Concurrency::parallel_for_each(consumers_.begin(), consumers_.end(), [&](const decltype(*consumers_.begin())& pair)
			{		
				try
				{
					if(!pair.second->send(frame))
						removables.push_back(pair.first);
				}
				catch(...)
				{		
					CASPAR_LOG_CURRENT_EXCEPTION();
					CASPAR_LOG(error) << "Consumer error. Trying to recover:" << pair.second->print();
					try
					{
						pair.second->initialize(format_desc_);
						pair.second->send(frame);
					}
					catch(...)
					{
						removables.push_back(pair.first);				
						CASPAR_LOG_CURRENT_EXCEPTION();
						CASPAR_LOG(error) << "Failed to recover consumer: " << pair.second->print() << L". Removing it.";
					}
				}
			});

			BOOST_FOREACH(auto& removable, removables)
				consumers_.erase(removable);		
		}
	}

private:
	
	bool has_synchronization_clock()
	{
		return std::any_of(consumers_.begin(), consumers_.end(), [](const decltype(*consumers_.begin())& p)
		{
			return p.second->has_synchronization_clock();
		});
	}
	
	std::wstring print() const
	{
		return L"output";
	}
};

output::output(output::source_t& source, const video_format_desc& format_desc) : impl_(new implementation(source, format_desc)){}
void output::add(int index, safe_ptr<frame_consumer>&& consumer){impl_->add(index, std::move(consumer));}
void output::remove(int index){impl_->remove(index);}
}}