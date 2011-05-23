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
#include "../StdAfx.h"

#include "destroy_producer_proxy.h"

#include <common/concurrency/executor.h>
#include <common/utility/assert.h>

namespace caspar { namespace core {
	
struct destroyer
{
	executor executor_;

	destroyer() : executor_(L"destroyer", true){}
	
	void destroy(safe_ptr<frame_producer>&& producer)
	{
		executor_.begin_invoke(std::bind(&destroyer::do_destroy, this, std::move(producer)));
	}

	void do_destroy(safe_ptr<frame_producer>& producer)
	{
		if(!producer.unique())
			CASPAR_LOG(warning) << producer->print() << L" Not destroyed on safe asynchronous destruction thread.";
		
		if(executor_.size() > 4)
			CASPAR_LOG(error) << L" Potential destroyer deadlock.";

		producer = frame_producer::empty();
	}

} g_destroyer;

destroy_producer_proxy::destroy_producer_proxy(const safe_ptr<frame_producer>& producer) : producer_(producer){}
destroy_producer_proxy::~destroy_producer_proxy()
{
	g_destroyer.destroy(std::move(producer_));
}

safe_ptr<basic_frame> destroy_producer_proxy::receive(){return core::receive(producer_);}  
std::wstring destroy_producer_proxy::print() const {return producer_->print();}   

void destroy_producer_proxy::param(const std::wstring& param){producer_->param(param);}  

safe_ptr<frame_producer> destroy_producer_proxy::get_following_producer() const {return producer_->get_following_producer();}  
void destroy_producer_proxy::set_leading_producer(const safe_ptr<frame_producer>& producer) {producer_->set_leading_producer(producer);} 
		
}}
