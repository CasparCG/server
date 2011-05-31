#ifndef _CASPAR_CGCONTROL_H__
#define _CASPAR_CGCONTROL_H__

#pragma once

namespace caspar {
namespace CG { 

class ICGControl
{
public:
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