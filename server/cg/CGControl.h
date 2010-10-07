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
 
#ifndef _CASPAR_CGCONTROL_H__
#define _CASPAR_CGCONTROL_H__

#pragma once

namespace caspar {
namespace CG { 

class ICGControl
{
public:
	virtual ~ICGControl() {}

	virtual void Add(int layer, const tstring& templateName, bool playOnLoad, const tstring& label, const tstring& data) = 0;
	virtual void Remove(int layer) = 0;
	virtual void Clear() = 0;
	virtual void Play(int layer) = 0;
	virtual void Stop(int layer, unsigned int mixOutDuration) = 0;
	virtual void Next(int layer) = 0;
	virtual void Update(int layer, const tstring& data) = 0;
	virtual void Invoke(int layer, const tstring& label) = 0;
};

}
}

#endif	//_CASPAR_CGCONTROL_H__