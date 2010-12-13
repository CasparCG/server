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

#include "../processor/draw_frame.h"
#include "../processor/frame_processor_device.h"
#include "../../common/utility/safe_ptr.h"

#include <boost/noncopyable.hpp>

#include <memory>

namespace caspar { namespace core {
	
class frame_producer : boost::noncopyable
{
public:
	virtual ~frame_producer(){}	

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual draw_frame :::receive() = 0;
	///
	/// \brief	Renders a frame.
	/// 		
	/// \note	This function is run in through the tbb task_schedular and shall be *non blocking*.
	///
	/// \return	The frame. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual safe_ptr<draw_frame> receive() = 0;

	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual std::shared_ptr<frame_producer> :::get_following_producer() const
	///
	/// \brief	Gets the producer which will replace the current producer on EOF. 
	///
	/// \return	The following producer, or nullptr if there is no following producer. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual safe_ptr<frame_producer> get_following_producer() const {return frame_producer::empty();}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual void :::set_leading_producer(const std::shared_ptr<frame_producer>& producer)
	///
	/// \brief	Sets the producer which was run before the current producer. 
	///
	/// \param	producer	The leading producer.
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void set_leading_producer(const safe_ptr<frame_producer>& /*producer*/) {}
	
	////////////////////////////////////////////////////////////////////////////////////////////////////
	/// \fn	virtual void :::initialize(const frame_processor_device_ptr& frame_processor) = 0;
	///
	/// \brief	Provides the frame frame_processor used to create frames and initializes the producer. 
	///
	/// \param	frame_processor	The frame frame_processor. 
	////////////////////////////////////////////////////////////////////////////////////////////////////
	virtual void initialize(const frame_processor_device_ptr& frame_processor) = 0;

	static safe_ptr<frame_producer> empty();

	virtual std::wstring print() const = 0;
};

inline std::wostream& operator<<(std::wostream& out, const frame_producer& producer);

}}
