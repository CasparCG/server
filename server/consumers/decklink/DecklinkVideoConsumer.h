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
#include "..\..\VideoConsumer.h"
#include "..\..\MediaProducer.h"

#include "..\..\utils\thread.h"
#include "..\..\utils\lockable.h"
#include "..\..\utils\Noncopyable.hpp"

#include "..\..\frame\Frame.h"

#include <boost/noncopyable.hpp>

namespace caspar { namespace decklink {

class DecklinkVideoConsumer : public IVideoConsumer, boost::noncopyable
{
public:
	DecklinkVideoConsumer(unsigned int device_index);
	virtual ~DecklinkVideoConsumer();
	
	virtual IPlaybackControl* GetPlaybackControl() const;

	virtual void EnableVideoOutput(){}
	virtual void DisableVideoOutput(){}
	virtual bool SetupDevice(unsigned int deviceIndex){return false;}
	virtual bool ReleaseDevice(){return false;}
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;
	virtual const TCHAR* GetFormatDescription() const;
	
	static int EnumerateDevices();
	bool SetVideoFormat(const tstring& strDesiredFrameFormat);
private:
	struct implementation;
	std::shared_ptr<implementation> impl_;
};

}	//namespace gdi
}	//namespace caspar