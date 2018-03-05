#include "av_input.h"

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <common/except.h>
#include <common/os/thread.h>
#include <common/scope_exit.h>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C"
{
#include <libavformat/avformat.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace caspar { namespace ffmpeg {

Input::Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph)
    : graph_(graph)
    , filename_(filename)
{
    graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));
    graph_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));

    thread_ = std::thread([=] {
        try {
            set_thread_name(L"[ffmpeg::av_producer::Input]");

            while (true) {
                {
                    std::unique_lock<std::mutex> lock(mutex_);
                    cond_.wait(lock, [&] {
                        return ic_ && ((!eof_ && !paused_) && output_.size() < output_capacity_) || abort_request_;
                    });
                }

                if (abort_request_) {
                    break;
                }

                std::lock_guard<std::mutex> format_lock(ic_mutex_);

                auto packet = alloc_packet();

                // TODO (perf) Non blocking av_read_frame when possible.
                auto ret = av_read_frame(ic_.get(), packet.get());

                {
                    std::lock_guard<std::mutex> lock(mutex_);

                    if (ret == AVERROR_EXIT) {
                        break;
                    } else if (ret == AVERROR_EOF) {
                        eof_   = true;
                        packet = nullptr;
                    } else {
                        FF_RET(ret, "av_read_frame");
                    }

                    output_.push(std::move(packet));
                    graph_->set_value("input", (static_cast<double>(output_.size() + 0.001) / output_capacity_));
                }
                cond_.notify_all();
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    });
}

Input::~Input()
{
    abort_request_ = true;
    cond_.notify_all();
    thread_.join();
}

int Input::interrupt_cb(void* ctx)
{
    auto input = reinterpret_cast<Input*>(ctx);
    return input->abort_request_ ? 1 : 0;
}

void Input::operator()(std::function<bool(std::shared_ptr<AVPacket>&)> fn)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (!output_.empty() && fn(output_.front())) {
            output_.pop();
        }
        graph_->set_value("input", (static_cast<double>(output_.size() + 0.001) / output_capacity_));
    }
    cond_.notify_all();
}

AVFormatContext* Input::operator->() { return ic_.get(); }
AVFormatContext* const Input::operator->() const { return ic_.get(); }

void Input::reset()
{
    AVDictionary* options = nullptr;
    CASPAR_SCOPE_EXIT{ av_dict_free(&options); };

    if (filename_.find("http://") == 0 || filename_.find("https://") == 0) {
        FF(av_dict_set(&options, "http_persistent", "0", 0));  // NOTE https://trac.ffmpeg.org/ticket/7034#comment:3
        FF(av_dict_set(&options, "http_multiple", "0", 0));    // NOTE https://trac.ffmpeg.org/ticket/7034#comment:3
        FF(av_dict_set(&options, "reconnect", "1", 0));        // HTTP reconnect
    }
                                                           // TODO (fix) timeout?
    FF(av_dict_set(&options, "rw_timeout", "60000000", 0)); // 60 second IO timeout

    AVFormatContext* ic = nullptr;
    FF(avformat_open_input(&ic, filename_.c_str(), nullptr, &options));
    ic_ = std::shared_ptr<AVFormatContext>(ic, [](AVFormatContext* ctx) { avformat_close_input(&ctx); });

    for (auto& p : to_map(&options)) {
        CASPAR_LOG(warning) << "av_input[" + filename_ + "]" << " Unused option " << p.first << "=" << p.second;
    }

    ic_->interrupt_callback.callback = Input::interrupt_cb;
    ic_->interrupt_callback.opaque = this;

    FF(avformat_find_stream_info(ic_.get(), nullptr));
}

boost::optional<int64_t> Input::start_time() const
{
    auto ic = ic_;
    return ic && ic->start_time != AV_NOPTS_VALUE ? ic->start_time : boost::optional<int64_t>();
}

boost::optional<int64_t> Input::duration() const
{
    auto ic = ic_;
    return ic && ic->duration != AV_NOPTS_VALUE ? ic->duration : boost::optional<int64_t>();
}

bool Input::paused() const { return paused_; }

void Input::paused(bool value)
{
    paused_ = value;
    cond_.notify_all();
}

bool Input::eof() const { return eof_; }

void Input::seek(int64_t ts, bool flush)
{
    std::lock_guard<std::mutex> lock(ic_mutex_);

    if (ts != ic_->start_time && ts != AV_NOPTS_VALUE) {
        FF(avformat_seek_file(ic_.get(), -1, INT64_MIN, ts, ts, 0));
    } else {
        reset();
    }

    {
        std::lock_guard<std::mutex> output_lock(mutex_);

        while (flush && !output_.empty()) {
            output_.pop();
        }
    }
    eof_ = false;
    cond_.notify_all();

    graph_->set_tag(diagnostics::tag_severity::INFO, "seek");
}

}} // namespace caspar::ffmpeg
