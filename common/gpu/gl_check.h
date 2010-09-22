#pragma once

#include <Glee.h>

#include "../exception/exceptions.h"

#include <boost/lexical_cast.hpp>

namespace caspar { namespace common { namespace gpu {

#ifdef _DEBUG

inline void DO_GL_CHECK()
{
	auto error = glGetError();
	if(error != GL_NO_ERROR)
		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(boost::lexical_cast<std::string>(glGetError())));
}

#define CASPAR_GL_CHECK(expr) expr; ::caspar::common::gpu::DO_GL_CHECK()
#else
#define CASPAR_GL_CHECK(expr) expr
#endif

}}}