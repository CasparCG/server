#include "StdAfx.h"

/*
* Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
*
* This file is part of CasparCG (www.casparcg.com).
*
* CasparCG is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* CasparCG is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
*
* Author: Robert Nagy, ronag89@gmail.com
*/
#include "ffmpeg_error.h"

#include <common/utf.h>
#include <common/log.h>

#pragma warning(disable: 4146)

extern "C"
{
#include <libavutil/error.h>
}

namespace caspar { namespace ffmpeg {

std::string av_error_str(int errn)
{
	char buf[256];
	memset(buf, 0, 256);
	if(av_strerror(errn, buf, 256) < 0)
		return "";
	return std::string(buf);
}

void throw_on_ffmpeg_error(int ret, const char* source, const char* func, const char* local_func, const char* file, int line)
{
	if(ret >= 0)
		return;

	switch(ret)
	{
	case AVERROR_BSF_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_bsf_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_DECODER_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_decoder_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_DEMUXER_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_demuxer_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_ENCODER_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_encoder_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_EOF:
        CASPAR_THROW_EXCEPTION(
			averror_eof()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_EXIT:
        CASPAR_THROW_EXCEPTION(
			averror_exit()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_FILTER_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_filter_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_MUXER_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_muxer_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_OPTION_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_option_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_PATCHWELCOME:
        CASPAR_THROW_EXCEPTION(
			averror_patchwelcome()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_PROTOCOL_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_protocol_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVERROR_STREAM_NOT_FOUND:
        CASPAR_THROW_EXCEPTION(
			averror_stream_not_found()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	case AVUNERROR(EINVAL):
        CASPAR_THROW_EXCEPTION(
			averror_invalid_argument()
				<< msg_info("Invalid FFmpeg argument given")
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	default:
        CASPAR_THROW_EXCEPTION(
			ffmpeg_error()
				<< msg_info(av_error_str(ret))
				<< source_info(source)
				<< boost::errinfo_api_function(func)
				<< boost::errinfo_errno(AVUNERROR(ret)));
	}
}

void throw_on_ffmpeg_error(int ret, const std::wstring& source, const char* func, const char* local_func, const char* file, int line)
{
	throw_on_ffmpeg_error(ret, u8(source).c_str(), func, local_func, file, line);
}

}}
