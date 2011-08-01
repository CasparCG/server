#pragma once

extern "C" 
{
	#define __STDC_CONSTANT_MACROS
	#define __STDC_LIMIT_MACROS
	#include <libavutil/cpu.h>
	#include <libavutil/common.h>
	#include <libavutil/pixdesc.h>
	#include <libavfilter/avfilter.h>
}

int init(AVFilterContext *ctx, const char *args, void *opaque);
void uninit(AVFilterContext *ctx);
int query_formats(AVFilterContext *ctx);
int poll_frame(AVFilterLink *link);
int request_frame(AVFilterLink *link);
void start_frame(AVFilterLink *link, AVFilterBufferRef *picref);
void end_frame(AVFilterLink *link);
void return_frame(AVFilterContext *ctx, int is_second);
AVFilterBufferRef *get_video_buffer(AVFilterLink *link, int perms, int w, int h);

void register_scalable_yadif();