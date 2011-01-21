#pragma once

#include "../log/log.h"

#ifdef _MSC_VER
#define _CASPAR_DBG_BREAK _CrtDbgBreak()
#else
#define _CASPAR_DBG_BREAK
#endif

#define CASPAR_ASSERT_EXPR_STR(str) #str

#define CASPAR_ASSERT(expr) do{if(!(expr)) CASPAR_LOG(warning) << "\n\nAssertion Failed:\n" << \
	CASPAR_ASSERT_EXPR_STR(expr) << "\n" \
	__FILE__ << "\n"; \
	_CASPAR_DBG_BREAK;\
	}while(0);