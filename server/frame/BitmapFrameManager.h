#ifndef _CASPAR_BITMAPFRAMEMANAGER_H__
#define _CASPAR_BITMAPFRAMEMANAGER_H__

#pragma once

#include "..\utils\Lockable.h"
#include "..\utils\Noncopyable.hpp"
#include "..\utils\BitmapHolder.h"

#include "FrameManager.h"

namespace caspar {

/*
	BitmapFrameManager

	Changes:
	2009-06-13 (R.N) : Refactored
*/

class BitmapFrameManager : public FrameManager, private utils::LockableObject, private utils::Noncopyable
{	
public:
	BitmapFrameManager(const FrameFormatDescription& fmtDesc, HWND hWnd);
	virtual ~BitmapFrameManager();

	virtual FramePtr CreateFrame();
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;

	virtual bool HasFeature(const std::string& feature) const;

private:
	// TODO: need proper threading tools (R.N)
	struct LockableBitmapCollection : public std::vector<BitmapHolderPtr>, public LockableObject{};
	typedef std::tr1::shared_ptr<LockableBitmapCollection> LockableBitmapCollectionPtr;

	class FrameDeallocator;
	LockableBitmapCollectionPtr pBitmaps_;

	std::vector<const std::string> features_;

	const FrameFormatDescription fmtDesc_;	
	const HWND hWnd_;
};
typedef std::tr1::shared_ptr<BitmapFrameManager> BitmapFrameManagerPtr;

}	//namespace caspar
#endif	