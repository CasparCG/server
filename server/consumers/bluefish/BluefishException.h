#ifndef _CASPAR_BLUEFISHEXCEPTION_H__
#define _CASPAR_BLUEFISHEXCEPTION_H__

#include <exception>

namespace caspar {
namespace bluefish {

class BluefishException : public std::exception
{
public:
	explicit BluefishException(const char* msg) : std::exception(msg) {}
	~BluefishException() {}
};

}	//namespace bluefish
}	//namespace caspar

#endif	//_CASPAR_BLUEFISHEXCEPTION_H__