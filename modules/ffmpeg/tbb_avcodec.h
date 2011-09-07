#pragma once

struct AVCodecContext;
struct AVCodec;

namespace caspar { namespace ffmpeg {
	
int tbb_avcodec_open(AVCodecContext *avctx, AVCodec *codec);
int tbb_avcodec_close(AVCodecContext *avctx);

}}