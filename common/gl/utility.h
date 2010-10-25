#pragma once

#include <Glee.h>

#include "../exception/exceptions.h"

#include <boost/lexical_cast.hpp>

namespace caspar { namespace common { namespace gpu {

#ifdef _DEBUG
	
#define CASPAR_GL_CHECK(expr) \
	do \
	{ \
		expr;  \
		auto error = glGetError(); \
		if(error != GL_NO_ERROR) \
			BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(boost::lexical_cast<std::string>(glGetError()))); \
	}while(0);
#else
#define CASPAR_GL_CHECK(expr) expr
#endif

}}}