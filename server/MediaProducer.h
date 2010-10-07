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

namespace caspar {

class IMediaController;

class MediaProducer;
typedef std::tr1::shared_ptr<MediaProducer> MediaProducerPtr;

class MediaProducer
{
	MediaProducer(const MediaProducer&);
	MediaProducer& operator=(const MediaProducer&);

public:
	MediaProducer() : bLoop_(false)
	{}
	virtual ~MediaProducer()
	{}

	virtual IMediaController* QueryController(const tstring&) = 0;

	virtual bool Param(const tstring&) { return false; }
	virtual bool IsEmpty() const { return false; }

	virtual MediaProducerPtr GetFollowingProducer() {
		return MediaProducerPtr();
	}

	void SetLoop(bool bLoop) {
		bLoop_ = bLoop;
	}
	bool GetLoop() {
		return bLoop_;
	}

private:
	bool bLoop_;
};

}	//namespace caspar