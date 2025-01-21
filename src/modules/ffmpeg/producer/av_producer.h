#include <memory>

#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/frame/geometry.h>
#include <core/monitor/monitor.h>
#include <core/video_format.h>

#include <memory>
#include <optional>
#include <string>

namespace caspar { namespace ffmpeg {

class AVProducer
{
  public:
    AVProducer(std::shared_ptr<core::frame_factory> frame_factory,
               core::video_format_desc              format_desc,
               std::string                          name,
               std::string                          path,
               std::optional<std::string>           vfilter,
               std::optional<std::string>           afilter,
               std::optional<int64_t>               start,
               std::optional<int64_t>               seek,
               std::optional<int64_t>               duration,
               std::optional<bool>                  loop,
               int                                  seekable,
               core::frame_geometry::scale_mode     scale_mode);

    core::draw_frame prev_frame(const core::video_field field);
    core::draw_frame next_frame(const core::video_field field);
    bool             is_ready();

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
