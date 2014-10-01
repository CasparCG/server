// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers

#define NOMINMAX
#include <windows.h>
#include <core/video_format.h>

#include <core/parameters/parameters.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame/basic_frame.h>
#include <core/producer/frame/frame_factory.h>
#include <core/mixer/write_frame.h>
#include <common/concurrency/future_util.h>

#include <common/env.h>
#include <common/log/log.h>
//#include "..\Include\utils.h"
//#include "..\Include\ErrorLog.h"
//#include "..\Include\XMLDefine.h"
//#include "..\include\logfile.h"
//#include "..\include\XMLDefine.h"
//#include "..\include\XMLParser.h"
//#include "..\include\Command.h"
//#include "..\Include\BeeMacros.h"
//#include "..\include\define.h"

#include <common/utility/base64.h>
#include <common/utility/string.h>

#include <boost/assign.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>

//

#include <algorithm>