#include "StdAfx.h"

#include "accelerator.h"

#ifdef _MSC_VER
#include "cpu/image/image_mixer.h"
#endif
#include "ogl/image/image_mixer.h"

#include "ogl/util/device.h"

#include <common/env.h>

#include <tbb/mutex.h>

namespace caspar { namespace accelerator {
	
struct accelerator::impl
{
	const std::wstring				path_;
	tbb::mutex						mutex_;
	std::shared_ptr<ogl::device>	ogl_device_;

	impl(const std::wstring& path)
		: path_(path)
	{
	}

	std::unique_ptr<core::image_mixer> create_image_mixer()
	{
		try
		{
			if(path_ == L"gpu" || path_ == L"ogl" || path_ == L"auto" || path_ == L"default")
			{
				tbb::mutex::scoped_lock lock(mutex_);

				if(!ogl_device_)
					ogl_device_.reset(new ogl::device());

				return std::unique_ptr<core::image_mixer>(new ogl::image_mixer(spl::make_shared_ptr(ogl_device_), env::properties().get(L"configuration.mixer.blend-modes", false)));
			}
		}
		catch(...)
		{
			if(path_ == L"gpu" || path_ == L"ogl")
				CASPAR_LOG_CURRENT_EXCEPTION();
		}
#ifdef _MSC_VER
		return std::unique_ptr<core::image_mixer>(new cpu::image_mixer());
#else
		CASPAR_THROW_EXCEPTION(not_supported());
#endif
	}
};

accelerator::accelerator(const std::wstring& path)
	: impl_(new impl(path))
{
}

accelerator::~accelerator()
{
}

std::unique_ptr<core::image_mixer> accelerator::create_image_mixer()
{
	return impl_->create_image_mixer();
}

}}
