#pragma once

#include "io\protocolStrategy.h"

namespace caspar {

class IController {
public:
	virtual ~IController() {
	}

	virtual bool Start() = 0;
	virtual void Stop() = 0;
	virtual void SetProtocolStrategy(caspar::IO::ProtocolStrategyPtr) = 0;
};
typedef std::tr1::shared_ptr<IController> ControllerPtr;

}	//namespace caspar