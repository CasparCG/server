#include "av_input.h"

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <common/except.h>
#include <common/os/thread.h>
#include <common/param.h>
#include <common/scope_exit.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/case_conv.hpp>

#include <set>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#include <libavformat/avformat.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace caspar { namespace ffmpeg {

Input::Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph, boost::optional<bool> seekable)
    : filename_(filename)
    , graph_(graph)
    , seekable_(seekable)
{
    graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));
    graph_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));

    buffer_.set_capacity(256);
    thread_ = std::thread([=] {
        try {
            set_thread_name(L"[ffmpeg::av_producer::Input]");

            while (true) {
                auto packet = alloc_packet();

                {
                    std::unique_lock<std::mutex> lock(ic_mutex_);
                    ic_cond_.wait(lock, [&] { return ic_ || abort_request_; });

                    if (abort_request_) {
                        break;
                    }

                    // TODO (perf) Non blocking av_read_frame when possible.
                    auto ret = av_read_frame(ic_.get(), packet.get());

                    if (ret == AVERROR_EXIT) {
                        break;
                    } else if (ret == AVERROR_EOF) {
                        eof_   = true;
                        packet = nullptr;
                    } else {
                        FF_RET(ret, "av_read_frame");
                    }
                }

                buffer_.push(std::move(packet));
                graph_->set_value("input", (static_cast<double>(buffer_.size()) / buffer_.capacity()));
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    });
}

Input::~Input()
{
    graph_         = spl::shared_ptr<diagnostics::graph>();
    abort_request_ = true;
    ic_cond_.notify_all();

    std::shared_ptr<AVPacket> packet;
    while (buffer_.try_pop(packet))
        ;

    thread_.join();
}

int Input::interrupt_cb(void* ctx)
{
    auto input = reinterpret_cast<Input*>(ctx);
    return input->abort_request_ ? 1 : 0;
}

bool Input::try_pop(std::shared_ptr<AVPacket>& packet)
{
    auto result = buffer_.try_pop(packet);
    graph_->set_value("input", (static_cast<double>(buffer_.size()) / buffer_.capacity()));
    return result;
}

AVFormatContext* Input::operator->() { return ic_.get(); }
AVFormatContext* const Input::operator->() const { return ic_.get(); }

void Input::abort()
{
    abort_request_ = true;
    ic_cond_.notify_all();
}

void Input::reset()
{
    std::unique_lock<std::mutex> lock(ic_mutex_);
    internal_reset();
}

void Input::internal_reset()
{
    AVDictionary* options = nullptr;
    CASPAR_SCOPE_EXIT { av_dict_free(&options); };

    static const std::set<std::wstring> PROTOCOLS_TREATED_AS_FORMATS = {L"dshow", L"v4l2", L"iec61883"};

    AVInputFormat* input_format = nullptr;
    auto           url_parts    = caspar::protocol_split(u16(filename_));
    if (url_parts.first == L"http" || url_parts.first == L"https") {
        FF(av_dict_set(&options, "multiple_requests", "1", 0)); // NOTE https://trac.ffmpeg.org/ticket/7034#comment:3
        FF(av_dict_set(&options, "reconnect", "1", 0));
        FF(av_dict_set(&options, "reconnect_streamed", "1", 0));
        FF(av_dict_set(&options, "reconnect_delay_max", "120", 0));
        FF(av_dict_set(&options, "referer", filename_.c_str(), 0)); // HTTP referer header
    } else if (url_parts.first == L"rtmp" || url_parts.first == L"rtmps") {
        FF(av_dict_set(&options, "rtmp_live", "live", 0));
    } else if (PROTOCOLS_TREATED_AS_FORMATS.find(url_parts.first) != PROTOCOLS_TREATED_AS_FORMATS.end()) {
        input_format = av_find_input_format(u8(url_parts.first).c_str());
        filename_    = u8(url_parts.second);
    }

    if (seekable_) {
        CASPAR_LOG(debug) << "av_input[" + filename_ + "] Disabled seeking";
        FF(av_dict_set(&options, "seekable", *seekable_ ? "1" : "0", 0));
    }

    if (input_format == nullptr) {
        // TODO (fix) timeout?
        FF(av_dict_set(&options, "rw_timeout", "60000000", 0)); // 60 second IO timeout
    }

    AVFormatContext* ic             = avformat_alloc_context();
    ic->interrupt_callback.callback = Input::interrupt_cb;
    ic->interrupt_callback.opaque   = this;

    FF(avformat_open_input(&ic, filename_.c_str(), input_format, &options));
    auto ic2 = std::shared_ptr<AVFormatContext>(ic, [](AVFormatContext* ctx) { avformat_close_input(&ctx); });

    for (auto& p : to_map(&options)) {
        CASPAR_LOG(warning) << "av_input[" + filename_ + "]"
                            << " Unused option " << p.first << "=" << p.second;
    }

    FF(avformat_find_stream_info(ic2.get(), nullptr));
    ic_ = std::move(ic2);
    ic_cond_.notify_all();
}

bool Input::eof() const { return eof_; }

void Input::seek(int64_t ts, bool flush)
{
    std::unique_lock<std::mutex> lock(ic_mutex_);

    if (ic_ && ts != ic_->start_time && ts != AV_NOPTS_VALUE) {
        FF(avformat_seek_file(ic_.get(), -1, INT64_MIN, ts, ts, 0));
    } else {
        internal_reset();
    }

    if (flush) {
        std::shared_ptr<AVPacket> packet;
        while (buffer_.try_pop(packet))
            ;
    }
    eof_ = false;

    graph_->set_tag(diagnostics::tag_severity::INFO, "seek");
}

}} // namespace caspar::ffmpeg
