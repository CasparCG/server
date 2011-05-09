/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
 
#pragma once

#ifndef _FFFMPEG_EXCEPTION_H_
#define _FFFMPEG_EXCEPTION_H_

#include <exception>
#include <system_error>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4482)
#endif

namespace caspar
{
	namespace ffmpeg
	{
		
namespace ffmpeg_errc
{
	enum ffmpeg_errc_t
	{
		unknown = 0,
		sws_failure = 604,

		invalid_file = 610,
			no_streams = 611,
			decoder_not_found = 612,
			invalid_codec = 613,
			unknown_codec = 614,
			invalid_video_codec = 615,
			invalid_audio_codec = 616,

		invalid_packet = 620,
			decode_failed = 621,
			decode_video_failed = 622,
			decode_audio_failed = 623,
			deinterlace_failed = 624,

		invalid_audio_frame_size = 630
	};
}

typedef ffmpeg_errc::ffmpeg_errc_t ffmpeg_errn;

inline std::error_code ffmpeg_make_averror_code(int averror)
{
	return std::make_error_code(static_cast<std::generic_errno>(-min(0, averror)));
}

class ffmpeg_category_impl : public std::error_category
{
public:
	virtual const char* name() const
	{
		return "ffmpeg";
	}

	virtual std::string message(int ev) const
	{ 
		switch (ev)
		{
		case ffmpeg_errn::decoder_not_found:
			return "Decoder for specified codec not found.";
		case ffmpeg_errn::unknown_codec:
			return "Codec not supported.";
		case ffmpeg_errn::invalid_video_codec:
			return "Could not open video codec.";
		case ffmpeg_errn::invalid_audio_codec:
			return "Could not open audio codec.";
		case ffmpeg_errn::decode_video_failed:
			return "Could not decode video packet.";
		case ffmpeg_errn::decode_audio_failed:
			return "Could not decode audio packet.";
		case ffmpeg_errn::deinterlace_failed:
			return "Could not deinterlace video packet. Make sure packet has not been scaled before deinterlacing.";	  
		}
		return "Unknown";
	}

	const std::error_category& ffmpeg_category()
	{
		static ffmpeg_category_impl instance;
		return instance;
	}

	std::error_code make_error_code(ffmpeg_errn e)
	{
		return std::error_code(static_cast<int>(e), ffmpeg_category());
	}

	std::error_condition make_error_condition(ffmpeg_errn e)
	{
		return std::error_condition(static_cast<int>(e), ffmpeg_category());
	}

	virtual bool equivalent(int code, const std::error_condition& condition) const
	{
		// TODO: Impletement
		return false;
	}

	virtual bool equivalent(const std::error_code& code, int condition) const
	{
		// TODO: Impletement
		return false;
	}

};

inline std::error_code ffmpeg_make_error_code(ffmpeg_errn errn)
{	
	return ffmpeg_category_impl().make_error_code(errn);
}

class ffmpeg_error : std::runtime_error
{
public:
	
	explicit ffmpeg_error(std::error_code error_code, const std::string& message = "")
		: 
		std::runtime_error(message), code_(error_code)
	{		
	}

	ffmpeg_error(std::error_code error_code, const char* message)
		: 
		std::runtime_error(message), code_(error_code)
	{
	}

	ffmpeg_error(std::error_code::value_type error_value, const std::error_category& error_category, const std::string& message = "")
		: 
		std::runtime_error(message), code_(error_value, error_category)
	{
	}

	ffmpeg_error(std::error_code::value_type error_value, const std::error_category& error_category, const char *message) 
		:
		std::runtime_error(message), code_(error_value, error_category)
	{
	}

	const std::error_code& code() const throw()
	{
		return code_;
	}

private:
	std::error_code code_;
};

	}

}

//namespace std
//{
//	template <>
//	struct std::is_error_code_enum<caspar::ffmpeg::ffmpeg_errn::ffmpeg_errn_t>: public std::true_type {};
//}


#if defined(_MSC_VER)
#pragma warning(pop)
#pragma warning(disable : 4482)
#endif

#endif