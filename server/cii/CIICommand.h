#pragma once

#include <vector>
#include <string>

namespace caspar {
namespace cii {

class ICIICommand
{
public:
	virtual ~ICIICommand() {}
	virtual int GetMinimumParameters() = 0;
	virtual void Setup(const std::vector<tstring>& parameters) = 0;

	virtual void Execute() = 0;
};
typedef std::tr1::shared_ptr<ICIICommand> CIICommandPtr;

}	//namespace cii
}	//namespace caspar