#pragma once

#include <common/except.h>

namespace caspar { namespace ffmpeg {

struct ffmpeg_error_t : virtual caspar_exception
{
};

using ffmpeg_errn_info = boost::error_info<struct tag_ffmpeg_errn_info, int>;

}
} // namespace caspar::ffmpeg

#define THROW_ON_ERROR_STR_(call) #call
#define THROW_ON_ERROR_STR(call) THROW_ON_ERROR_STR_(call)

#define FF_RET(ret, func)                                                                                              \
    if (ret < 0) {                                                                                                     \
        CASPAR_THROW_EXCEPTION(caspar::ffmpeg::ffmpeg_error_t()                                                        \
                               << boost::errinfo_api_function(func)                                                    \
                               << caspar::ffmpeg::ffmpeg_errn_info(ret)                                                \
                               << boost::errinfo_errno(AVUNERROR(ret)));                                               \
    }

#define FF(call)                                                                                                       \
    [&] {                                                                                                              \
        auto ret = call;                                                                                               \
        FF_RET(ret, THROW_ON_ERROR_STR(call));                                                                         \
    }()

#define FFMEM(call)                                                                                                    \
    [&] {                                                                                                              \
        auto val = call;                                                                                               \
        if (!val)                                                                                                      \
            FF_RET(AVERROR(ENOMEM), THROW_ON_ERROR_STR(call))                                                          \
        return std::move(val);                                                                                         \
    }()
