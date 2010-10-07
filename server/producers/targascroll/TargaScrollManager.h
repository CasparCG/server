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
 
#ifndef TARGASCROLLMEIDAMANAGER_H
#define TARGASCROLLMEIDAMANAGER_H

#include "..\..\MediaManager.h"

namespace caspar {

class TargaScrollMediaProducer;

class TargaScrollMediaManager : public IMediaManager
{
	public:
		TargaScrollMediaManager();
		virtual ~TargaScrollMediaManager();

		virtual bool getFileInfo(FileInfo* pFileInfo);
		virtual MediaProducerPtr CreateProducer(const tstring& filename);

	private:
		typedef std::tr1::shared_ptr<TargaScrollMediaProducer> TargaScrollMediaProducerPtr;
};

}

#endif