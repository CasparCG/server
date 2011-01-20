#pragma once

#include "../log/log.h"

#define __ASSERT_EXPR_STR(name) #name

#define CASPAR_ASSERT(expr) do{if(!(expr)) CASPAR_LOG(error) << "\nAssertion failed.\n " << __ASSERT_EXPR_STR(expr) << "\n" << __FILE__ << "\n" << __LINE__ << "\n\n";}while(0)