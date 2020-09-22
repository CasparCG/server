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

#include <tbb/concurrent_queue.h>

#include <boost/optional.hpp>

struct AVPacket;
struct AVFormatContext;

namespace caspar { namespace ffmpeg {

class Input
{
  public:
    Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph, boost::optional<bool> seekable);
    ~Input();

    static int interrupt_cb(void* ctx);

    bool try_pop(std::shared_ptr<AVPacket>& packet);

    AVFormatContext* operator->();

    AVFormatContext* const operator->() const;

    void reset();
    void abort();
    bool eof() const;
    void seek(int64_t ts, bool flush = true);

  private:
    void internal_reset();

    boost::optional<bool> seekable_;

    std::string                         filename_;
    std::shared_ptr<diagnostics::graph> graph_;

    mutable std::mutex               ic_mutex_;
    std::shared_ptr<AVFormatContext> ic_;
    std::condition_variable          ic_cond_;

    tbb::concurrent_bounded_queue<std::shared_ptr<AVPacket>> buffer_;

    std::atomic<bool> eof_{false};

    std::atomic<bool> abort_request_{false};
    std::thread       thread_;
};

}} // namespace caspar::ffmpeg
