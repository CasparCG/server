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
 
#ifndef _CASPAR_FRAMEMANAGER_H__
#define _CASPAR_FRAMEMANAGER_H__

#include "Frame.h"

#include <vector>
#include <string>

#include "..\utils\ID.h"

namespace caspar {

class FrameManager : public utils::Identifiable
{
public:
	virtual ~FrameManager() {}
	virtual FramePtr CreateFrame() = 0;
	virtual const FrameFormatDescription& GetFrameFormatDescription() const = 0;
	virtual bool HasFeature(const std::string& feature) const
	{
		return false;
	}
	bool Owns(const Frame& frame) const
	{
		return (frame.FactoryID() == ID());
	}
};
typedef std::tr1::shared_ptr<FrameManager> FrameManagerPtr;

}	//namespace caspar

#endif	//_CASPAR_FRAMEMANAGER_H__