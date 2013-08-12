#pragma once

#include "../util/lock_container.h"
#include <core/video_channel.h>
#include <common/memory.h>

namespace caspar { namespace protocol { namespace amcp {

class channel_context
{
	channel_context();
public:
	explicit channel_context(const spl::shared_ptr<core::video_channel>& c, const std::wstring& lifecycle_key) : channel(c), lock(spl::make_shared<caspar::IO::lock_container>(lifecycle_key)) {}
	spl::shared_ptr<core::video_channel>		channel;
	caspar::IO::lock_container::ptr_type		lock;
};

}}}