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
 
#ifndef __VIDEO_CONSUMER_H__
#define __VIDEO_CONSUMER_H__

namespace caspar {

class IPlaybackControl;
class MediaProducer;

class IVideoConsumer
{
public:
	virtual ~IVideoConsumer() {}
	virtual IPlaybackControl* GetPlaybackControl() const = 0;
	virtual void EnableVideoOutput() = 0;
	virtual void DisableVideoOutput() = 0;
	virtual bool SetupDevice(unsigned int deviceIndex) = 0;
	virtual bool ReleaseDevice() = 0;
	virtual const TCHAR* GetFormatDescription() const = 0;

	virtual bool SetVideoFormat(const tstring& strDesiredFrameFormat)
	{
		LOG << TEXT("SetVideoFormat is no supported");
		return false; 
	}
};
typedef std::tr1::shared_ptr<IVideoConsumer> VideoConsumerPtr;

}	//namespace caspar
#endif //__VIDEO_CONSUMER_H__