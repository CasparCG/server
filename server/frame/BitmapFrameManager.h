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
 
#ifndef _CASPAR_BITMAPFRAMEMANAGER_H__
#define _CASPAR_BITMAPFRAMEMANAGER_H__

#include "..\utils\Noncopyable.hpp"

#include "FrameManager.h"
#include "BitmapFrame.h"

namespace caspar {

class BitmapFrameManager : public FrameManager, public utils::LockableObject, private utils::Noncopyable
{	
public:
	BitmapFrameManager(const FrameFormatDescription& fmtDesc, HWND hWnd);
	virtual ~BitmapFrameManager();

	virtual FramePtr CreateFrame();
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;

	virtual bool HasFeature(const std::string& feature) const;

private:

	virtual BitmapFramePtr CreateBitmapFrame();

	struct Implementation;
	std::tr1::shared_ptr<Implementation> pImpl_;
};
typedef std::tr1::shared_ptr<BitmapFrameManager> BitmapFrameManagerPtr;

}	//namespace caspar
#endif	