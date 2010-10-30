#pragma once

class CBlueVelvet4;

namespace caspar { namespace core { namespace bluefish {

typedef std::tr1::shared_ptr<CBlueVelvet4> BlueVelvetPtr;

class consumer;
typedef std::shared_ptr<consumer> consumer_ptr;

}}}