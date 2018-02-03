#pragma once

#include <memory>
#include <string>

namespace caspar { namespace accelerator {

class accelerator
{
public:
	accelerator(const std::wstring& path);
    accelerator(accelerator&) = delete;
	~accelerator();

    accelerator& operator=(accelerator&) = delete;

	std::unique_ptr<core::image_mixer> create_image_mixer(int channel_id);
private:
	struct impl;
	std::unique_ptr<impl> impl_;
};

}}