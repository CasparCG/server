#pragma once

#include <common/diagnostics/graph.h>

#include <string>
#include <memory>
#include <cstdint>
#include <functional>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <thread>

struct AVPacket;
struct AVFormatContext;

namespace caspar {
namespace ffmpeg {

class Input
{
public:
    Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph);
    ~Input();

    static int interrupt_cb(void* ctx);

    void operator()(std::function<bool(std::shared_ptr<AVPacket>&)> fn);

    AVFormatContext* operator->();

    AVFormatContext* const operator->() const;

    int64_t start_time() const;

    bool paused() const;

    bool eof() const;

    void pause();

    void resume();

    void seek(int64_t ts, bool flush = true);
private:
    std::shared_ptr<diagnostics::graph> graph_;

    mutable std::mutex               ic_mutex_;
    std::shared_ptr<AVFormatContext> ic_;

    mutable std::mutex                    mutex_;
    std::condition_variable               cond_;
    int                                   output_capacity_ = 64;
    std::queue<std::shared_ptr<AVPacket>> output_;

    std::atomic<bool>                     paused_ = false;
    std::atomic<bool>                     eof_ = false;

    std::atomic<bool> abort_request_ = false;
    std::thread       thread_;
};

} }