#include "av_util.h"

#if defined(_MSC_VER)
#pragma warning (push)
#pragma warning (disable : 4244)
#endif
extern "C" {
#include <libavutil/channel_layout.h>
#include <libavutil/pixfmt.h>
#include <libavutil/frame.h>
#include <libavcodec/avcodec.h>
}
#if defined(_MSC_VER)
#pragma warning (pop)
#endif

namespace caspar {
namespace ffmpeg2 {

core::audio_channel_layout get_audio_channel_layout(int num_channels, std::uint64_t layout)
{
	// What FFmpeg calls "channel layout" is only the "layout type" of a channel layout in
	// CasparCG where the channel layout supports different orders as well.
	// The user needs to provide additional mix-configs in casparcg.config to support more
	// than the most common (5.1, mono and stereo) types.

	// Based on information in https://ffmpeg.org/ffmpeg-utils.html#Channel-Layout
	switch (layout)
	{
	case AV_CH_LAYOUT_MONO:
		return core::audio_channel_layout(num_channels, L"mono",			L"FC");
	case AV_CH_LAYOUT_STEREO:
		return core::audio_channel_layout(num_channels, L"stereo",			L"FL FR");
	case AV_CH_LAYOUT_2POINT1:
		return core::audio_channel_layout(num_channels, L"2.1",				L"FL FR LFE");
	case AV_CH_LAYOUT_SURROUND:
		return core::audio_channel_layout(num_channels, L"3.0",				L"FL FR FC");
	case AV_CH_LAYOUT_2_1:
		return core::audio_channel_layout(num_channels, L"3.0(back)",		L"FL FR BC");
	case AV_CH_LAYOUT_4POINT0:
		return core::audio_channel_layout(num_channels, L"4.0",				L"FL FR FC BC");
	case AV_CH_LAYOUT_QUAD:
		return core::audio_channel_layout(num_channels, L"quad",			L"FL FR BL BR");
	case AV_CH_LAYOUT_2_2:
		return core::audio_channel_layout(num_channels, L"quad(side)",		L"FL FR SL SR");
	case AV_CH_LAYOUT_3POINT1:
		return core::audio_channel_layout(num_channels, L"3.1",				L"FL FR FC LFE");
	case AV_CH_LAYOUT_5POINT0_BACK:
		return core::audio_channel_layout(num_channels, L"5.0",				L"FL FR FC BL BR");
	case AV_CH_LAYOUT_5POINT0:
		return core::audio_channel_layout(num_channels, L"5.0(side)",		L"FL FR FC SL SR");
	case AV_CH_LAYOUT_4POINT1:
		return core::audio_channel_layout(num_channels, L"4.1",				L"FL FR FC LFE BC");
	case AV_CH_LAYOUT_5POINT1_BACK:
		return core::audio_channel_layout(num_channels, L"5.1",				L"FL FR FC LFE BL BR");
	case AV_CH_LAYOUT_5POINT1:
		return core::audio_channel_layout(num_channels, L"5.1(side)",		L"FL FR FC LFE SL SR");
	case AV_CH_LAYOUT_6POINT0:
		return core::audio_channel_layout(num_channels, L"6.0",				L"FL FR FC BC SL SR");
	case AV_CH_LAYOUT_6POINT0_FRONT:
		return core::audio_channel_layout(num_channels, L"6.0(front)",		L"FL FR FLC FRC SL SR");
	case AV_CH_LAYOUT_HEXAGONAL:
		return core::audio_channel_layout(num_channels, L"hexagonal",		L"FL FR FC BL BR BC");
	case AV_CH_LAYOUT_6POINT1:
		return core::audio_channel_layout(num_channels, L"6.1",				L"FL FR FC LFE BC SL SR");
	case AV_CH_LAYOUT_6POINT1_BACK:
		return core::audio_channel_layout(num_channels, L"6.1(back)",		L"FL FR FC LFE BL BR BC");
	case AV_CH_LAYOUT_6POINT1_FRONT:
		return core::audio_channel_layout(num_channels, L"6.1(front)",		L"FL FR LFE FLC FRC SL SR");
	case AV_CH_LAYOUT_7POINT0:
		return core::audio_channel_layout(num_channels, L"7.0",				L"FL FR FC BL BR SL SR");
	case AV_CH_LAYOUT_7POINT0_FRONT:
		return core::audio_channel_layout(num_channels, L"7.0(front)",		L"FL FR FC FLC FRC SL SR");
	case AV_CH_LAYOUT_7POINT1:
		return core::audio_channel_layout(num_channels, L"7.1",				L"FL FR FC LFE BL BR SL SR");
	case AV_CH_LAYOUT_7POINT1_WIDE_BACK:
		return core::audio_channel_layout(num_channels, L"7.1(wide)",		L"FL FR FC LFE BL BR FLC FRC");
	case AV_CH_LAYOUT_7POINT1_WIDE:
		return core::audio_channel_layout(num_channels, L"7.1(wide-side)",	L"FL FR FC LFE FLC FRC SL SR");
	case AV_CH_LAYOUT_STEREO_DOWNMIX:
		return core::audio_channel_layout(num_channels, L"downmix",			L"DL DR");
	default:
		// Passthru
		return core::audio_channel_layout(num_channels, L"", L"");
	}
}

core::pixel_format get_pixel_format(AVPixelFormat pix_fmt) 
{
    switch (pix_fmt) 
    {
    case AVPixelFormat::AV_PIX_FMT_GRAY8:       return core::pixel_format::gray;
    case AVPixelFormat::AV_PIX_FMT_RGB24:       return core::pixel_format::rgb;
    case AVPixelFormat::AV_PIX_FMT_BGR24:       return core::pixel_format::bgr;
    case AVPixelFormat::AV_PIX_FMT_BGRA:        return core::pixel_format::bgra;
    case AVPixelFormat::AV_PIX_FMT_ARGB:        return core::pixel_format::argb;
    case AVPixelFormat::AV_PIX_FMT_RGBA:        return core::pixel_format::rgba;
    case AVPixelFormat::AV_PIX_FMT_ABGR:        return core::pixel_format::abgr;
    case AVPixelFormat::AV_PIX_FMT_YUV444P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV422P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV420P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV411P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUV410P:     return core::pixel_format::ycbcr;
    case AVPixelFormat::AV_PIX_FMT_YUVA420P:    return core::pixel_format::ycbcra;
    case AVPixelFormat::AV_PIX_FMT_YUVA422P:    return core::pixel_format::ycbcra;
    case AVPixelFormat::AV_PIX_FMT_YUVA444P:    return core::pixel_format::ycbcra;
    default:                                    return core::pixel_format::invalid;
    }
}

core::pixel_format_desc pixel_format_desc(AVPixelFormat pix_fmt, int width, int height) 
{
    // Get linesizes
    AVPicture dummy_pict;
    avpicture_fill(&dummy_pict, nullptr, pix_fmt, width, height);

    core::pixel_format_desc desc = get_pixel_format(pix_fmt);

    switch (desc.format) 
    {
    case core::pixel_format::gray:
    case core::pixel_format::luma: 
    {
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
        return desc;
    }
    case core::pixel_format::bgr:
    case core::pixel_format::rgb: 
    {
    desc.planes.push_back(core::pixel_format_desc::plane(
        dummy_pict.linesize[0] / 3, height, 3));
        return desc;
    }
    case core::pixel_format::bgra:
    case core::pixel_format::argb:
    case core::pixel_format::rgba:
    case core::pixel_format::abgr: 
    {
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0] / 4, height, 4));
        return desc;
    }
    case core::pixel_format::ycbcr:
    case core::pixel_format::ycbcra: 
    {
        // Find chroma height
        auto size2 = static_cast<int>(dummy_pict.data[2] - dummy_pict.data[1]);
        auto h2 = size2 / dummy_pict.linesize[1];

        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[0], height, 1));
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[1], h2, 1));
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[2], h2, 1));

        if (desc.format == core::pixel_format::ycbcra)
        desc.planes.push_back(core::pixel_format_desc::plane(dummy_pict.linesize[3], height, 1));

        return desc;
    }
    default:
        desc.format = core::pixel_format::invalid;
        return desc;
    }
}

}
}