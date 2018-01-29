#pragma once

#include <boost/exception/all.hpp>

#include <exception>
#include <string>

namespace caspar {
namespace ffmpeg2 {

struct ffmpeg_error_t : virtual boost::exception, virtual std::exception {};
    
}  // namespace ffmpeg
}  // namespace caspar

#define THROW_ON_ERROR_STR_(call) #call
#define THROW_ON_ERROR_STR(call) THROW_ON_ERROR_STR_(call)

#define FF_RET(ret, func)                                         \
  if (ret < 0) {                                                  \
    BOOST_THROW_EXCEPTION(caspar::ffmpeg2::ffmpeg_error_t()       \
        << boost::errinfo_api_function(func)                      \
        << boost::errinfo_errno(AVUNERROR(static_cast<int>(ret))) \
    );                                                            \
  }

#define FF(call) FF_RET(call, THROW_ON_ERROR_STR(call))

#define FFMEM(call) [&] {                                     \
  auto val = call;                                            \
  if (!val) FF_RET(AVERROR(ENOMEM), THROW_ON_ERROR_STR(call)) \
  return std::move(val);                                      \
}()

