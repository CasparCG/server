#pragma once

#include <core/producer/frame_producer.h>

#include <string>
#include <vector>

namespace caspar { 
namespace core {
	class parameters;
}
namespace WASP {

safe_ptr<core::frame_producer> create_producer(
		const safe_ptr<core::frame_factory>& frame_factory,
		const core::parameters& param);
safe_ptr<core::frame_producer> create_thumbnail_producer(
		const safe_ptr<core::frame_factory>& frame_factory,
		const core::parameters& param);

}}