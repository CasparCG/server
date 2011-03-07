#pragma once

namespace caspar {

void init_ffmpeg();

std::wstring get_avcodec_version();
std::wstring get_avformat_version();
std::wstring get_swscale_version();

}