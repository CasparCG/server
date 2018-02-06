#pragma once

#include <atomic>
#include <condition_variable>
#include <memory>
#include <queue>

struct AVPacket;
struct AVFrame;
struct AVStream;
struct AVCodecContext;

namespace caspar { namespace ffmpeg {

class Decoder
{
  public:
    Decoder(AVStream* stream);

    ~Decoder();

    AVCodecContext* operator->();

    const AVCodecContext* operator->() const;

    bool try_push(std::shared_ptr<AVPacket> packet);

    void operator()(std::function<bool(std::shared_ptr<AVFrame>&)> fn);

    void flush();

  private:
    mutable std::mutex              ctx_mutex_;
    std::shared_ptr<AVCodecContext> ctx_;

    int64_t next_pts_;

    mutable std::mutex                    mutex_;
    std::condition_variable               cond_;
    int                                   input_capacity_ = 256;
    std::queue<std::shared_ptr<AVPacket>> input_;
    int                                   output_capacity_ = 2;
    std::queue<std::shared_ptr<AVFrame>>  output_;

    std::atomic<bool> abort_request_{ false };
    std::thread       thread_;
};

}} // namespace caspar::ffmpeg