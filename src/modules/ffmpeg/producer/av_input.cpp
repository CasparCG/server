#include "av_input.h"

#include "../util/av_assert.h"
#include "../util/av_util.h"

#include <common/env.h>
#include <common/except.h>
#include <common/os/thread.h>
#include <common/param.h>
#include <common/scope_exit.h>

#include <boost/algorithm/string/case_conv.hpp>
#include <boost/filesystem.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/regex.hpp>

#include <algorithm>
#include <set>

extern "C" {
#include <libavformat/avformat.h>
}

namespace caspar { namespace ffmpeg {

namespace {

// Parses ffmpeg command line style options ("-key value ...") into an AVDictionary. Every option
// needs a value: a value-less "-flag" followed by another option takes that option as its value.
void set_options(AVDictionary** options, const std::string& str)
{
    static const boost::regex opt_exp("-(?<NAME>[^\\s]+)(\\s+(?<VALUE>[^\\s]+))?");

    for (auto it = boost::sregex_iterator(str.begin(), str.end(), opt_exp); it != boost::sregex_iterator(); ++it) {
        const auto name  = (*it)["NAME"].str();
        const auto value = (*it)["VALUE"].matched ? (*it)["VALUE"].str() : std::string();
        FF(av_dict_set(options, name.c_str(), value.c_str(), 0));
    }
}

// Options from <ffmpeg><producer><options><scope>. Unknown options are logged as unused once the
// input has been opened, but an invalid value for a known option makes the open fail.
void set_config_options(AVDictionary** options, const std::wstring& scope)
{
    const auto str = env::properties().get<std::wstring>(L"configuration.ffmpeg.producer.options." + scope, L"");
    if (!str.empty()) {
        set_options(options, u8(str));
    }
}

} // namespace

Input::Input(const std::string& filename, std::shared_ptr<diagnostics::graph> graph, std::optional<bool> seekable)
    : filename_(filename)
    , graph_(graph)
    , seekable_(seekable)
{
    graph_->set_color("seek", diagnostics::color(1.0f, 0.5f, 0.0f));
    graph_->set_color("input", diagnostics::color(0.7f, 0.4f, 0.4f));

    buffer_.set_capacity(256);
    thread_ = boost::thread([this] {
        try {
            set_thread_name(L"[ffmpeg::av_producer::Input]");

            int consecutive_errors = 0;
            int reopen_attempts    = 0;

            while (true) {
                auto packet = alloc_packet();
                int  ret    = 0;
                bool live   = false;

                {
                    std::unique_lock<std::mutex> lock(ic_mutex_);
                    ic_cond_.wait(lock, [&] { return ic_ || abort_request_; });

                    if (abort_request_) {
                        break;
                    }

                    // TODO (perf) Non blocking av_read_frame when possible.
                    ret  = av_read_frame(ic_.get(), packet.get());
                    live = !ic_->pb || !(ic_->pb->seekable & AVIO_SEEKABLE_NORMAL);
                }

                if (abort_request_) {
                    break;
                }

                if (ret == AVERROR_EXIT) {
                    break;
                } else if (ret == AVERROR(EAGAIN)) {
                    boost::this_thread::yield();
                    continue;
                } else if (ret == AVERROR_EOF) {
                    eof_   = true;
                    packet = nullptr;
                } else if (ret < 0) {
                    ++consecutive_errors;
                    if (consecutive_errors == 1) {
                        char msg[AV_ERROR_MAX_STRING_SIZE] = {};
                        av_strerror(ret, msg, sizeof(msg));
                        CASPAR_LOG(warning) << "av_input[" << filename_ << "] av_read_frame: " << msg << ", retrying";
                    }
                    if (live && consecutive_errors >= 5) {
                        // Some protocol errors are sticky (e.g. a UDP buffer overrun), so a live input
                        // is reopened until it delivers again or the producer goes away.
                        if (!reopen(reopen_attempts)) {
                            break;
                        }
                        consecutive_errors = 0;
                        continue;
                    }
                    if (!live && consecutive_errors >= 20) {
                        CASPAR_LOG(error)
                            << "av_input[" << filename_ << "] av_read_frame: too many consecutive errors, aborting";

                        // Pretend we reached EOF, to avoid the producer stalling expecting more packets
                        eof_   = true;
                        packet = nullptr;
                        buffer_.push(std::move(packet));
                        break;
                    }
                    boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
                    continue;
                }

                consecutive_errors = 0;
                reopen_attempts    = 0;
                buffer_.push(std::move(packet));
                graph_->set_value("input", (static_cast<double>(buffer_.size()) / buffer_.capacity()));
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
            boost::this_thread::sleep_for(boost::chrono::milliseconds(5));
        }
    });
}

Input::~Input()
{
    request_abort();

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

AVFormatContext*       Input::operator->() { return ic_.get(); }
AVFormatContext* const Input::operator->() const { return ic_.get(); }

void Input::request_abort()
{
    // The flag is set first: interrupt_cb reads it without the lock, so a read or open holding the
    // lock returns promptly. Taking the lock before notifying means a waiter cannot test the flag,
    // miss this notification and then block for good.
    abort_request_ = true;
    {
        std::lock_guard<std::mutex> lock(ic_mutex_);
    }
    ic_cond_.notify_all();
}

void Input::abort()
{
    request_abort();

    std::shared_ptr<AVPacket> packet;
    while (buffer_.try_pop(packet))
        ;
}

bool Input::reopen(int& attempt)
{
    while (true) {
        ++attempt;
        std::unique_lock<std::mutex> lock(ic_mutex_);
        const auto                   backoff = std::chrono::milliseconds(std::min(100 * attempt, 2000));
        if (ic_cond_.wait_for(lock, backoff, [&] { return abort_request_.load(); })) {
            return false;
        }
        try {
            // Release the old connection, or reopening a UDP input fails to bind the port it still
            // holds. Nothing reads from the old context in the meantime.
            if (ic_ && ic_->pb && !(ic_->iformat->flags & AVFMT_NOFILE)) {
                avio_closep(&ic_->pb);
            }
            reopened_     = open();
            has_reopened_ = true;
            CASPAR_LOG(info) << "av_input[" << filename_ << "] reopened";
        } catch (...) {
            if (attempt == 1 || attempt % 30 == 0) {
                CASPAR_LOG_CURRENT_EXCEPTION();
            }
            continue;
        }
        // The producer's decoders refer to the old context's streams, so the producer switches
        // contexts itself, once it has dropped them.
        ic_cond_.wait(lock, [&] { return !reopened_ || abort_request_; });
        return !abort_request_;
    }
}

bool Input::reopened() const { return has_reopened_; }

void Input::use_reopened()
{
    std::unique_lock<std::mutex> lock(ic_mutex_);
    if (reopened_) {
        // Packets still queued were read from the old context; its streams may not match the new
        // ones. The reader is waiting, so nothing new can arrive before the switch.
        std::shared_ptr<AVPacket> packet;
        while (buffer_.try_pop(packet))
            ;
        ic_           = std::move(reopened_);
        has_reopened_ = false;
        ic_cond_.notify_all();
    }
}

void Input::reset()
{
    std::unique_lock<std::mutex> lock(ic_mutex_);
    internal_reset();
}

void Input::internal_reset()
{
    ic_ = open();
    ic_cond_.notify_all();
}

std::shared_ptr<AVFormatContext> Input::open()
{
    AVDictionary* options = nullptr;
    CASPAR_SCOPE_EXIT { av_dict_free(&options); };

    static const std::set<std::wstring> PROTOCOLS_TREATED_AS_FORMATS = {L"dshow", L"v4l2", L"iec61883"};

    const AVInputFormat* input_format = nullptr;
    auto                 url          = filename_;
    auto                 url_parts    = caspar::protocol_split(u16(filename_));
    if (url_parts.first == L"http" || url_parts.first == L"https") {
        FF(av_dict_set(&options, "multiple_requests", "1", 0)); // NOTE https://trac.ffmpeg.org/ticket/7034#comment:3
        FF(av_dict_set(&options, "reconnect", "1", 0));
        FF(av_dict_set(&options, "reconnect_streamed", "1", 0));
        FF(av_dict_set(&options, "reconnect_delay_max", "120", 0));
        FF(av_dict_set(&options, "referer", filename_.c_str(), 0)); // HTTP referer header
    } else if (url_parts.first == L"rtmp" || url_parts.first == L"rtmps") {
        FF(av_dict_set(&options, "rtmp_live", "live", 0));
    } else if (PROTOCOLS_TREATED_AS_FORMATS.find(url_parts.first) != PROTOCOLS_TREATED_AS_FORMATS.end()) {
        // filename_ keeps the protocol, so that a reopen selects the device format again.
        input_format = av_find_input_format(u8(url_parts.first).c_str());
        url          = u8(url_parts.second);
    }

    if (seekable_) {
        CASPAR_LOG(debug) << "av_input[" + filename_ + "] Disabled seeking";
        FF(av_dict_set(&options, "seekable", *seekable_ ? "1" : "0", 0));
    }

    if (input_format == nullptr) {
        // TODO (fix) timeout?
        FF(av_dict_set(&options, "rw_timeout", "60000000", 0)); // 60 second IO timeout
    }

    // Applied last so that user configuration wins over the defaults above.
    set_config_options(&options, L"default");
    if (!url_parts.first.empty()) {
        // The TLS variants share their protocol implementation, so they share its scope as well.
        auto scope = url_parts.first;
        if (scope == L"https" || scope == L"rtmps") {
            scope.pop_back();
        }
        set_config_options(&options, scope);
    }

    AVFormatContext* ic             = avformat_alloc_context();
    ic->interrupt_callback.callback = Input::interrupt_cb;
    ic->interrupt_callback.opaque   = this;

    FF(avformat_open_input(&ic, url.c_str(), input_format, &options));
    auto ic2 = std::shared_ptr<AVFormatContext>(ic, [](AVFormatContext* ctx) { avformat_close_input(&ctx); });

    for (auto& p : to_map(&options)) {
        CASPAR_LOG(warning) << "av_input[" + filename_ + "]" << " Unused option " << p.first << "=" << p.second;
    }

    FF(avformat_find_stream_info(ic2.get(), nullptr));
    return ic2;
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
