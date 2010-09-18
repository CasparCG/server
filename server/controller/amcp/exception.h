#pragma once

#include "../../../common/exception/exceptions.h"

namespace caspar { namespace controller { namespace amcp {
	
struct invalid_paremeter : virtual caspar_exception{};
struct invalid_channel : virtual invalid_paremeter{};
struct missing_parameter : virtual caspar_exception{};

}}}