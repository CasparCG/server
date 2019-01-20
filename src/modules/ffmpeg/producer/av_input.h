#pragma once

#include <common/diagnostics/graph.h>

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>

struct AVPacket;
struct AVFormatContext;

namespace caspar { namespace ffmpeg {

class Input
{
  public:
    Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph);
    ~Input();

    static int interrupt_cb(void* ctx);

    void operator()(std::function<bool(std::shared_ptr<AVPacket>&)> fn);

    AVFormatContext* operator->();

    AVFormatContext* const operator->() const;

    void reset();
    bool eof() const;
    void abort();

    void seek(int64_t ts, bool flush = true);

  private:
    std::string                         filename_;
    std::shared_ptr<diagnostics::graph> graph_;

    mutable std::mutex               ic_mutex_;
    std::shared_ptr<AVFormatContext> ic_;

    mutable std::mutex                    mutex_;
    std::condition_variable               cond_;
    std::size_t                           output_capacity_ = 256;
    std::queue<std::shared_ptr<AVPacket>> output_;

    std::atomic<bool> eof_{false};

    std::atomic<bool> abort_request_{false};
    std::thread       thread_;
};

}} // namespace caspar::ffmpeg
