#pragma once

#include <string>

namespace caspar {
namespace ffmpeg {

enum FFMPEG_Resource {
	FFMPEG_FILE,
	FFMPEG_DEVICE,
	FFMPEG_STREAM
};

struct ffmpeg_params
{
	std::wstring  size_str;
	std::wstring  pixel_format;
	std::wstring  frame_rate;


	ffmpeg_params() 
		: size_str(L"")
		, pixel_format(L"")
		, frame_rate(L"")
	{
	}

};

}}