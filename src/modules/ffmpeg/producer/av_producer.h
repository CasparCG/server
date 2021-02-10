#include <memory>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/monitor/monitor.h>

#include <boost/optional.hpp>

#include <memory>
#include <string>

namespace caspar { namespace ffmpeg {

class AVProducer
{
  public:
    AVProducer(std::shared_ptr<core::frame_factory> frame_factory,
               core::video_format_desc              format_desc,
               std::string                          name,
               std::string                          path,
               boost::optional<std::string>         vfilter,
               boost::optional<std::string>         afilter,
               boost::optional<int64_t>             start,
               boost::optional<int64_t>             duration,
               boost::optional<bool>                loop,
               int                                  seekable);

    core::draw_frame prev_frame();
    core::draw_frame next_frame();

    AVProducer& seek(int64_t time);
    int64_t     time() const;

    AVProducer& loop(bool loop);
    bool        loop() const;

    AVProducer& start(int64_t start);
    int64_t     start() const;

    AVProducer& duration(int64_t duration);
    int64_t     duration() const;

    caspar::core::monitor::state state() const;

  private:
    struct Impl;
    std::shared_ptr<Impl> impl_;
};

}} // namespace caspar::ffmpeg
