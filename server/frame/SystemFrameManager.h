#ifndef _CASPAR_SYSTEMFRAMEMANAGER_H__
#define _CASPAR_SYSTEMFRAMEMANAGER_H__

#pragma once

#include "..\utils\Allocator.h"
#include "..\utils\Noncopyable.hpp"

#include "FrameManager.h"

#include <unordered_map>

namespace caspar {

class Frame;
typedef std::tr1::shared_ptr<Frame> FramePtr;

/*
	SystemFrameManager

	Changes:
	2009-06-14 (R.N) : Refactored, note: Is thread-safe since "FixedAllocator" is thread-safe
*/

class SystemFrameManager : public FrameManager, private utils::Noncopyable, private utils::LockableObject
{
public:
	explicit SystemFrameManager(const FrameFormatDescription&);
	virtual ~SystemFrameManager();

	virtual FramePtr CreateFrame();
	virtual const FrameFormatDescription& GetFrameFormatDescription() const;

private:
	class FrameDeallocator;
	const FrameFormatDescription fmtDesc_;
	utils::FixedAllocatorPtr pAllocator_;

	class LockableAllocatorAssoc : public std::tr1::unordered_map<size_t, utils::FixedAllocatorPtr>, public utils::LockableObject{};

	static LockableAllocatorAssoc allocators;
};
typedef std::tr1::shared_ptr<SystemFrameManager> SystemFrameManagerPtr;

}	//namespace caspar
#endif	//_CASPAR_SYSTEMFRAMEMANAGER_H__