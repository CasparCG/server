#pragma once

#define NOMINMAX

#if defined(_MSC_VER)
#	ifndef _SCL_SECURE_NO_WARNINGS
#		define _SCL_SECURE_NO_WARNINGS
#	endif
#	ifndef _CRT_SECURE_NO_WARNINGS
#		define _CRT_SECURE_NO_WARNINGS
#	endif
#endif


#define CASPAR_VERSION_STR	"2.0.0.1"
#define CASPAR_VERSION_TAG	"EXPERIMENTAL"

//#define CASPAR_TRACE_UNDERFLOW

#define TBB_USE_THREADING_TOOLS 1

#define BOOST_PARAMETER_MAX_ARITY 7

//#define DISABLE_BLUEFISH
