#pragma once

#include "../../../common/exception/exceptions.h"

namespace caspar { namespace controller { namespace protocol { namespace amcp {
	
struct amcp_error : virtual caspar_exception{};
struct invalid_paremeter : virtual amcp_error{};
struct invalid_channel : virtual invalid_paremeter{};
struct missing_parameter : virtual amcp_error{};

}}}}