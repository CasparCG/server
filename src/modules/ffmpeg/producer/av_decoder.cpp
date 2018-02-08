#include "av_decoder.h"

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
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/error.h>
#include <libavutil/opt.h>
}
#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace caspar { namespace ffmpeg {

Decoder::Decoder(AVStream* stream)
    : next_pts_(AV_NOPTS_VALUE)
{
    const auto codec = avcodec_find_decoder(stream->codecpar->codec_id);
    if (!codec) {
        FF_RET(AVERROR_DECODER_NOT_FOUND, "avcodec_find_decoder");
    }

    ctx_ = std::shared_ptr<AVCodecContext>(avcodec_alloc_context3(codec),
                                           [](AVCodecContext* ptr) { avcodec_free_context(&ptr); });

    if (!ctx_) {
        FF_RET(AVERROR(ENOMEM), "avcodec_alloc_context3");
    }

    FF(avcodec_parameters_to_context(ctx_.get(), stream->codecpar));

    FF(av_opt_set_int(ctx_.get(), "refcounted_frames", 1, 0));
    FF(av_opt_set_int(ctx_.get(), "threads", 4, 0));

    ctx_->pkt_timebase = stream->time_base;

    if (ctx_->codec_type == AVMEDIA_TYPE_VIDEO) {
        ctx_->framerate           = av_guess_frame_rate(nullptr, stream, nullptr);
        ctx_->sample_aspect_ratio = av_guess_sample_aspect_ratio(nullptr, stream, nullptr);
    } else if (ctx_->codec_type == AVMEDIA_TYPE_AUDIO) {
        if (!ctx_->channel_layout && ctx_->channels) {
            ctx_->channel_layout = av_get_default_channel_layout(ctx_->channels);
        }
        if (!ctx_->channels && ctx_->channel_layout) {
            ctx_->channels = av_get_channel_layout_nb_channels(ctx_->channel_layout);
        }
    }

    FF(avcodec_open2(ctx_.get(), codec, nullptr));

    thread_ = std::thread([=] {
        try {
            set_thread_name(L"[ffmpeg::av_producer::Stream]");

            while (true) {
                {
                    std::unique_lock<std::mutex> lock(mutex_);

                    cond_.wait(
                        lock, [&] { return (!input_.empty() && output_.size() < output_capacity_) || abort_request_; });
                }

                if (abort_request_) {
                    break;
                }

                std::lock_guard<std::mutex> decoder_lock(ctx_mutex_);

                {
                    if (!input_.empty()) {
                        std::shared_ptr<AVPacket> packet;
                        {
                            std::unique_lock<std::mutex> lock(mutex_);
                            packet = input_.front();
                        }

                        auto ret = avcodec_send_packet(ctx_.get(), packet.get());
                        if (ret == AVERROR(EAGAIN)) {
                            break;
                        } else {
                            FF_RET(ret, "avcodec_send_packet");
                            std::unique_lock<std::mutex> lock(mutex_);
                            input_.pop();
                        }
                    }
                    cond_.notify_all();
                }

                // TODO (perf) Don't receive all frames only the amount needed for output_capacity_.
                while (true) {
                    auto frame = alloc_frame();
                    auto ret   = avcodec_receive_frame(ctx_.get(), frame.get());

                    if (ret == AVERROR(EAGAIN)) {
                        break;
                    } else if (ret == AVERROR_EOF) {
                        avcodec_flush_buffers(ctx_.get());
                        frame->pts = next_pts_;
                    } else {
                        FF_RET(ret, "avcodec_receive_frame");

                        // TODO (fix) is this always best?
                        frame->pts = frame->best_effort_timestamp;
                        // TODO (fix) is this always best?
                        next_pts_ = frame->pts + frame->pkt_duration;
                    }

                    {
                        std::lock_guard<std::mutex> lock(mutex_);
                        output_.push(std::move(frame));
                    }
                    cond_.notify_all();
                }
            }
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    });
}

Decoder::~Decoder()
{
    abort_request_ = true;
    cond_.notify_all();
    thread_.join();
}

AVCodecContext* Decoder::operator->() { return ctx_.get(); }

const AVCodecContext* Decoder::operator->() const { return ctx_.get(); }

bool Decoder::try_push(std::shared_ptr<AVPacket> packet)
{
    {
        std::lock_guard<std::mutex> lock(mutex_);

        if (input_.size() > input_capacity_ && packet) {
            return false;
        }
        if (!packet) {
            // TODO (fix) remove pending null range
        }
        input_.push(std::move(packet));
    }
    cond_.notify_all();

    return true;
}

void Decoder::operator()(std::function<bool(std::shared_ptr<AVFrame>&)> fn)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);

        while (!output_.empty() && fn(output_.front())) {
            output_.pop();
        }
    }
    cond_.notify_all();
}

void Decoder::flush()
{
    std::lock_guard<std::mutex> decoder_lock(ctx_mutex_);

    avcodec_flush_buffers(ctx_.get());
    next_pts_ = AV_NOPTS_VALUE;

    {
        std::lock_guard<std::mutex> lock(mutex_);

        while (!output_.empty()) {
            output_.pop();
        }
        while (!input_.empty()) {
            input_.pop();
        }
    }
    cond_.notify_all();
}

}} // namespace caspar::ffmpeg