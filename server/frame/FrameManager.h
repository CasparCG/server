#ifndef _CASPAR_FRAMEMANAGER_H__
#define _CASPAR_FRAMEMANAGER_H__

#include "Frame.h"

#include <vector>
#include <string>

#include "..\utils\ID.h"

namespace caspar {

/*
	FrameManager

	Changes:
	2009-06-08 (R.N) : Added "HashCode" and "Owns" whuch allows frame to see which factory (framemanager) it belongs to
*/

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