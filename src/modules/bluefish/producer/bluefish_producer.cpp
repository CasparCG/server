/*
 * Copyright (c) 2011 Sveriges Television AB <info@casparcg.com>
 *
 * This file is part of CasparCG (www.casparcg.com).
 *
 * CasparCG is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * CasparCG is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with CasparCG. If not, see <http://www.gnu.org/licenses/>.
 *
 * Author:Robert Nagy, ronag89@gmail.com
 *		 satchit puthenveetil
 *        James Wise, james.wise@bluefish444.com
 */

#include "../StdAfx.h"

#include "bluefish_producer.h"

#include "../util/blue_velvet.h"
#include "../util/memory.h"

#include <common/diagnostics/graph.h>
#include <common/except.h>
#include <common/executor.h>
#include <common/log.h>
#include <common/param.h>
#include <common/scope_exit.h>
#include <common/timer.h>

#include <ffmpeg/util/av_util.h>

#include <core/diagnostics/call_context.h>
#include <core/frame/draw_frame.h>
#include <core/frame/frame_factory.h>
#include <core/monitor/monitor.h>
#include <core/producer/frame_producer.h>

#include <tbb/concurrent_queue.h>

#include <boost/algorithm/string.hpp>
#include <boost/range/adaptor/transformed.hpp>

#include <boost/circular_buffer.hpp>
#include <boost/format.hpp>
#include <boost/range/algorithm.hpp>

#include <mutex>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4244)
#endif
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/pixfmt.h>
#include <libavutil/samplefmt.h>
#include <libavutil/timecode.h>
}
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

using namespace caspar::ffmpeg;

namespace caspar { namespace bluefish {

static const size_t MAX_DECODED_AUDIO_BUFFER_SIZE = 2002 * 16; // max 2002 samples, 16 channels.

unsigned int get_bluesdk_input_videochannel_from_streamid(int stream_id)
{
    /*This function would return the corresponding EBlueVideoChannel from them stream_id argument */
    switch (stream_id) {
        case 1:
            return BLUE_VIDEO_INPUT_CHANNEL_1;
        case 2:
            return BLUE_VIDEO_INPUT_CHANNEL_2;
        case 3:
            return BLUE_VIDEO_INPUT_CHANNEL_3;
        case 4:
            return BLUE_VIDEO_INPUT_CHANNEL_4;
        case 5:
            return BLUE_VIDEO_INPUT_CHANNEL_5;
        case 6:
            return BLUE_VIDEO_INPUT_CHANNEL_6;
        case 7:
            return BLUE_VIDEO_INPUT_CHANNEL_7;
        case 8:
            return BLUE_VIDEO_INPUT_CHANNEL_8;
        default:
            return BLUE_VIDEO_INPUT_CHANNEL_1;
    }
}

unsigned int extract_pcm_data_from_hanc(bvc_wrapper&               blue,
                                        struct hanc_decode_struct* decode_struct,
                                        unsigned int               card_type,
                                        unsigned int*              src_hanc_buffer,
                                        unsigned int*              pcm_audio_buffer,
                                        int                        audio_channels_to_extract)
{
    decode_struct->audio_pcm_data_ptr              = pcm_audio_buffer;
    decode_struct->type_of_sample_required         = 0; // No flags indicates default of 32bit samples.
    decode_struct->max_expected_audio_sample_count = 2002;

    if (audio_channels_to_extract == 2)
        decode_struct->audio_ch_required_mask = MONO_CHANNEL_1 | MONO_CHANNEL_2;
    else if (audio_channels_to_extract == 8)
        decode_struct->audio_ch_required_mask = MONO_CHANNEL_1 | MONO_CHANNEL_2 | MONO_CHANNEL_3 | MONO_CHANNEL_4 |
                                                MONO_CHANNEL_5 | MONO_CHANNEL_6 | MONO_CHANNEL_7 | MONO_CHANNEL_8;
    else if (audio_channels_to_extract == 16)
        decode_struct->audio_ch_required_mask = MONO_CHANNEL_1 | MONO_CHANNEL_2 | MONO_CHANNEL_3 | MONO_CHANNEL_4 |
                                                MONO_CHANNEL_5 | MONO_CHANNEL_6 | MONO_CHANNEL_7 | MONO_CHANNEL_8 |
                                                MONO_CHANNEL_11 | MONO_CHANNEL_12 | MONO_CHANNEL_13 | MONO_CHANNEL_14 |
                                                MONO_CHANNEL_15 | MONO_CHANNEL_16 | MONO_CHANNEL_17 | MONO_CHANNEL_18;

    blue.decode_hanc_frame(card_type, src_hanc_buffer, decode_struct);
    return decode_struct->no_audio_samples;
}

bool is_video_format_interlaced(const core::video_format format)
{
    bool interlaced = false;
    if (format == core::video_format::x1080i5000 || format == core::video_format::x1080i5994 ||
        format == core::video_format::x1080i6000 || format == core::video_format::pal ||
        format == core::video_format::ntsc)
        interlaced = true;

    return interlaced;
}

bool is_bluefish_format_interlaced(unsigned int vid_mode)
{
    bool interlaced = false;
    if (vid_mode == VID_FMT_EXT_PAL || vid_mode == VID_FMT_EXT_NTSC || vid_mode == VID_FMT_EXT_1080I_5000 ||
        vid_mode == VID_FMT_EXT_1080I_5994 || vid_mode == VID_FMT_EXT_1080I_6000)
        interlaced = true;

    return interlaced;
}

struct bluefish_producer
{
    const int                    device_index_;
    const int                    stream_index_;
    spl::shared_ptr<bvc_wrapper> blue_;

    core::monitor::state                state_;
    mutable std::mutex                  state_mutex_;
    spl::shared_ptr<diagnostics::graph> graph_;
    caspar::timer                       tick_timer_;
    caspar::timer                       processing_benchmark_timer_;

    std::vector<int>   audio_cadence_;
    const std::wstring model_name_ = std::wstring(L"bluefish");

    std::atomic_bool                   process_capture_ = true;
    std::shared_ptr<std::thread>       capture_thread_;
    std::array<blue_dma_buffer_ptr, 1> reserved_frames_;

    core::video_format_desc format_desc_;
    core::video_format_desc channel_format_desc_;
    unsigned int            mode_;

    spl::shared_ptr<core::frame_factory> frame_factory_;
    std::vector<uint8_t>                 conversion_buffer_;

    tbb::concurrent_bounded_queue<core::draw_frame> frame_buffer_;
    std::exception_ptr                              exception_;

    ULONG schedule_capture_frame_id_   = 0;
    ULONG capturing_frame_id_          = std::numeric_limits<ULONG>::max();
    ULONG dma_ready_captured_frame_id_ = std::numeric_limits<ULONG>::max();

    struct hanc_decode_struct hanc_decode_struct_;
    std::vector<uint32_t>     decoded_audio_bytes_;
    unsigned int              memory_format_on_card_;
    unsigned int              sync_format_;
    bool                      first_frame_              = true;
    int                       frames_captured           = 0;
    uint64_t                  capture_ts                = 0;
    int                       remainaing_audio_samples_ = 0;
    int uhd_mode_ = 0; // 0 -> Do Not Allow BVC-ML, 1 -> Auto ( ie. Native buffers will do default mode, or BVC will do
                       // SQ.),  2 -> Force 2SI, 3 -> Force SQ

    bluefish_producer(const bluefish_producer&) = delete;
    bluefish_producer& operator=(const bluefish_producer&) = delete;

    bluefish_producer(const core::video_format_desc&              format_desc,
                      int                                         device_index,
                      int                                         stream_index,
                      int                                         uhd_mode,
                      const spl::shared_ptr<core::frame_factory>& frame_factory)
        : device_index_(device_index)
        , stream_index_(stream_index)
        , blue_(create_blue(device_index))
        , model_name_(get_card_desc(*blue_, device_index))
        , channel_format_desc_(format_desc)
        , frame_factory_(frame_factory)
        , memory_format_on_card_(MEM_FMT_RGB)
        , sync_format_(UPD_FMT_FRAME)
        , uhd_mode_(uhd_mode)
    {
        mode_ = static_cast<unsigned int>(VID_FMT_EXT_INVALID);
        frame_buffer_.set_capacity(2);

        graph_->set_color("tick-time", diagnostics::color(0.0f, 0.6f, 0.9f));
        graph_->set_color("late-frame", diagnostics::color(0.6f, 0.3f, 0.3f));
        graph_->set_color("frame-time", diagnostics::color(1.0f, 0.0f, 0.0f));
        graph_->set_color("dropped-frame", diagnostics::color(0.3f, 0.6f, 0.3f));
        graph_->set_color("output-buffer", diagnostics::color(0.0f, 1.0f, 0.0f));
        graph_->set_text(print());
        diagnostics::register_graph(graph_);

        std::memset(&hanc_decode_struct_, 0, sizeof(hanc_decode_struct_));
        hanc_decode_struct_.audio_input_source = AUDIO_INPUT_SOURCE_EMB;

        decoded_audio_bytes_.resize(MAX_DECODED_AUDIO_BUFFER_SIZE);

        // Configure input connector and routing
        unsigned int bf_channel = get_bluesdk_input_videochannel_from_streamid(stream_index);
        if (blue_->set_card_property32(DEFAULT_VIDEO_INPUT_CHANNEL, bf_channel))
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set input channel."));

        if (configure_input_routing(bf_channel, true, uhd_mode_)) // to do: tofix pass rgba vs use actual dual link!!
            CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set input routing."));

        // Look for a valid input video mode
        blue_->get_card_property32(VIDEO_MODE_EXT_INPUT, &mode_);
        if (mode_ != VID_FMT_EXT_INVALID) {
            if (uhd_mode_ == 1 || uhd_mode_ == 2 || uhd_mode_ == 3) {
                blue_->set_multilink(device_index_, bf_channel);
            }

            blue_->get_card_property32(VIDEO_MODE_EXT_INPUT, &mode_);
            format_desc_ = get_format_desc(
                *blue_, static_cast<EVideoModeExt>(mode_), static_cast<EMemoryFormat>(memory_format_on_card_));
            audio_cadence_ = format_desc_.audio_cadence;

            if (format_desc_.size == 0) {
                CASPAR_LOG(warning) << print()
                                    << TEXT("Problem getting the size of video buffer from SDK, calculating instead.");
                format_desc_.size = format_desc_.width * format_desc_.height * 3;
            }

            // Select input memory format
            if (blue_->set_card_property32(VIDEO_INPUT_MEMORY_FORMAT, memory_format_on_card_))
                CASPAR_THROW_EXCEPTION(caspar_exception()
                                       << msg_info(print() + L" Failed to set input memory format."));

            // Select image orientation
            if (blue_->set_card_property32(VIDEO_INPUTFRAMESTORE_IMAGE_ORIENTATION, ImageOrientation_Normal))
                CASPAR_LOG(warning) << print() << L" Failed to set image orientation to normal.";

            // Select data range   - // todo: confirm if we want to pass CGR or SMPTE range to caspar!!!!
            if (blue_->set_card_property32(EPOCH_VIDEO_INPUT_RGB_DATA_RANGE, CGR_RANGE))
                CASPAR_LOG(warning) << print() << L" Failed to set RGB data range to CGR.";

            // If we have an interlaced input AND and interlaced project then we need to handle the incoming frames
            // differently and sync on fields, instead of frame barriers
            if (is_video_format_interlaced(format_desc.format) && is_bluefish_format_interlaced(mode_))
                sync_format_ = UPD_FMT_FIELD;

            // Select Update Mode for input
            // HERE: this *might need to be sync format, but
            // currently we will leave as Frame UPD
            if (blue_->set_card_property32(VIDEO_INPUT_UPDATE_TYPE, UPD_FMT_FRAME))
                CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(print() + L" Failed to set input update type."));

            // Generate dma buffers
            int n = 0;
            boost::range::generate(reserved_frames_, [&] {
                return std::make_shared<blue_dma_buffer>(static_cast<int>(format_desc_.size), n++);
            });

            // Allocate a single UHD buffer for converison Buffer if we need it! .
            if (uhd_mode_ == 2) {
                conversion_buffer_.resize(static_cast<int>(format_desc_.size));
            }

            // Set Video Engine
            if (blue_->set_card_property32(VIDEO_INPUT_ENGINE, VIDEO_ENGINE_FRAMESTORE))
                CASPAR_LOG(warning) << print() << TEXT(" Failed to set video engine.");

            capture_thread_ = std::make_shared<std::thread>([this] { capture_thread_actual(); });
        }
    }

    int configure_input_routing(const unsigned int bf_channel, bool dual_link, int uhd_mode)
    {
        unsigned int routing_value   = 0;
        unsigned int routing_value_b = 0;

        /*This function would return the corresponding EBlueVideoChannel from the device output channel*/
        switch (bf_channel) {
            case BLUE_VIDEO_INPUT_CHANNEL_1:
                if (dual_link) {
                    routing_value = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_1, EPOCH_DEST_INPUT_MEM_INTERFACE_CH1, BLUE_CONNECTOR_PROP_DUALLINK_LINK_1);
                    routing_value_b = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_2, EPOCH_DEST_INPUT_MEM_INTERFACE_CH1, BLUE_CONNECTOR_PROP_DUALLINK_LINK_2);
                } else
                    routing_value = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_1, EPOCH_DEST_INPUT_MEM_INTERFACE_CH1, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                break;
            case BLUE_VIDEO_INPUT_CHANNEL_2:
                if (dual_link) {
                    routing_value = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_2, EPOCH_DEST_INPUT_MEM_INTERFACE_CH2, BLUE_CONNECTOR_PROP_DUALLINK_LINK_1);
                    routing_value_b = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_3, EPOCH_DEST_INPUT_MEM_INTERFACE_CH2, BLUE_CONNECTOR_PROP_DUALLINK_LINK_2);
                } else
                    routing_value = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_2, EPOCH_DEST_INPUT_MEM_INTERFACE_CH2, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                break;
            case BLUE_VIDEO_INPUT_CHANNEL_3:
                if (dual_link) {
                    routing_value = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_3, EPOCH_DEST_INPUT_MEM_INTERFACE_CH3, BLUE_CONNECTOR_PROP_DUALLINK_LINK_1);
                    routing_value_b = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_4, EPOCH_DEST_INPUT_MEM_INTERFACE_CH3, BLUE_CONNECTOR_PROP_DUALLINK_LINK_2);
                } else
                    routing_value = EPOCH_SET_ROUTING(
                        EPOCH_SRC_SDI_INPUT_3, EPOCH_DEST_INPUT_MEM_INTERFACE_CH3, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                break;
            case BLUE_VIDEO_INPUT_CHANNEL_4:
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_4, EPOCH_DEST_INPUT_MEM_INTERFACE_CH4, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                break;
            default:
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_1, EPOCH_DEST_INPUT_MEM_INTERFACE_CH1, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                break;
        }

        if (dual_link) {
            blue_->set_card_property32(MR2_ROUTING, routing_value);
            return blue_->set_card_property32(MR2_ROUTING, routing_value_b);
        }

        if (((uhd_mode == 1) || (uhd_mode == 2) || (uhd_mode == 3)) &&
            ((bf_channel == BLUE_VIDEO_INPUT_CHANNEL_1) || (bf_channel == BLUE_VIDEO_INPUT_CHANNEL_5))) {
            // Configure the input routing for the first 4 input channels
            if (bf_channel == BLUE_VIDEO_INPUT_CHANNEL_1) {
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_1, EPOCH_DEST_INPUT_MEM_INTERFACE_CH1, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_2, EPOCH_DEST_INPUT_MEM_INTERFACE_CH2, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_3, EPOCH_DEST_INPUT_MEM_INTERFACE_CH3, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_4, EPOCH_DEST_INPUT_MEM_INTERFACE_CH4, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
            } else {
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_5, EPOCH_DEST_INPUT_MEM_INTERFACE_CH5, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_6, EPOCH_DEST_INPUT_MEM_INTERFACE_CH6, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_7, EPOCH_DEST_INPUT_MEM_INTERFACE_CH7, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
                routing_value = EPOCH_SET_ROUTING(
                    EPOCH_SRC_SDI_INPUT_8, EPOCH_DEST_INPUT_MEM_INTERFACE_CH8, BLUE_CONNECTOR_PROP_SINGLE_LINK);
                return blue_->set_card_property32(MR2_ROUTING, routing_value);
            }
        } else
            return blue_->set_card_property32(MR2_ROUTING, routing_value);
    }

    void schedule_capture()
    {
        if (sync_format_ == UPD_FMT_FRAME || (sync_format_ == UPD_FMT_FIELD && !first_frame_)) {
            blue_->render_buffer_capture(BlueBuffer_Image_HANC(schedule_capture_frame_id_));
            dma_ready_captured_frame_id_ = capturing_frame_id_;
            capturing_frame_id_          = schedule_capture_frame_id_;
            schedule_capture_frame_id_   = (schedule_capture_frame_id_ + 1) % 4;
        }
    }

    void get_capture_time() { blue_->get_card_property64(BTC_TIMER, &capture_ts); }

    HRESULT process_data()
    {
        caspar::timer frame_timer;

        // Get info for source video mode
        unsigned int width          = 0;
        unsigned int height         = 0;
        unsigned int rate           = 0;
        unsigned int is_1001        = 0;
        unsigned int is_progressive = 0;
        unsigned int image_size     = 0;
        blue_->get_frame_info_for_video_mode(mode_, &width, &height, &rate, &is_1001, &is_progressive);
        blue_->get_bytes_per_frame(static_cast<EVideoModeExt>(mode_),
                                   static_cast<EMemoryFormat>(memory_format_on_card_),
                                   static_cast<EUpdateMethod>(sync_format_),
                                   &image_size);
        double fps = rate;
        if (is_1001 != 0u)
            fps = static_cast<double>(rate) * 1000 / 1001;

        CASPAR_SCOPE_EXIT
        {
            {
                std::lock_guard<std::mutex> lock(state_mutex_);
                state_["file/name"]              = model_name_;
                state_["file/path"]              = device_index_;
                state_["file/video/width"]       = static_cast<long>(width);
                state_["file/video/height"]      = static_cast<long>(height);
                state_["file/audio/sample-rate"] = format_desc_.audio_sample_rate;
                state_["file/audio/channels"]    = format_desc_.audio_channels;
                state_["file/fps"]               = static_cast<double>(fps);
                state_["profiler/time"]          = {frame_timer.elapsed(), fps};
                state_["buffer"]                 = {frame_buffer_.size(), frame_buffer_.capacity()};
            }

            graph_->set_value("frame-time", frame_timer.elapsed() * fps / format_desc_.field_count * 0.5);
            graph_->set_value("output-buffer",
                              static_cast<float>(frame_buffer_.size()) / static_cast<float>(frame_buffer_.capacity()));
        };

        try {
            graph_->set_value("tick-time", tick_timer_.elapsed() * fps * 0.5);
            tick_timer_.restart();
            {
                auto src_video = alloc_frame();
                auto src_audio = alloc_frame();

                // video
                src_video->format                 = AV_PIX_FMT_RGB24;
                src_video->width                  = width;
                src_video->height                 = height;
                src_video->interlaced_frame       = !is_progressive;
                src_video->top_field_first        = height != 486;
                src_video->key_frame              = 1;
                src_video->display_picture_number = frames_captured;
                src_video->pts                    = capture_ts;

                void* video_bytes = reserved_frames_.front()->image_data();
                if (reserved_frames_.front() && video_bytes) {
                    src_video->data[0]     = reinterpret_cast<uint8_t*>(reserved_frames_.front()->image_data());
                    src_video->linesize[0] = static_cast<int>(width * 3); // image_size / height);
                }

                // Audio
                src_audio->format      = AV_SAMPLE_FMT_S32;
                src_audio->channels    = format_desc_.audio_channels;
                src_audio->sample_rate = format_desc_.audio_sample_rate;
                src_audio->nb_samples  = 0;
                int samples_decoded    = 0;

                // hmm is audio on first frame or do we need to wait till snd feild to get audio?
                if (sync_format_ == UPD_FMT_FRAME || (sync_format_ == UPD_FMT_FIELD && first_frame_)) {
                    void* audio_bytes = nullptr;
                    auto  hanc_buffer = reinterpret_cast<uint8_t*>(reserved_frames_.front()->hanc_data());
                    if (hanc_buffer) {
                        int card_type = CRD_INVALID;
                        blue_->query_card_type(&card_type, device_index_);
                        auto no_extracted_pcm_samples =
                            extract_pcm_data_from_hanc(*blue_,
                                                       &hanc_decode_struct_,
                                                       card_type,
                                                       reinterpret_cast<unsigned int*>(hanc_buffer),
                                                       reinterpret_cast<unsigned int*>(&decoded_audio_bytes_[0]),
                                                       src_audio->channels);

                        audio_bytes = reinterpret_cast<int32_t*>(&decoded_audio_bytes_[0]);

                        samples_decoded       = no_extracted_pcm_samples / src_audio->channels;
                        src_audio->nb_samples = samples_decoded;
                        src_audio->data[0]    = reinterpret_cast<uint8_t*>(audio_bytes);
                        src_audio->linesize[0] =
                            src_audio->nb_samples * src_audio->channels *
                            av_get_bytes_per_sample(static_cast<AVSampleFormat>(src_audio->format));
                        src_audio->pts = capture_ts;
                    }
                }

                if (sync_format_ == UPD_FMT_FIELD) {
                    // since we provide an entire frame for each field in interlaced modes, we need to adjust the
                    // src_audio
                    if (first_frame_) {
                        remainaing_audio_samples_ = src_audio->nb_samples - src_audio->nb_samples / 2;
                        src_audio->nb_samples     = src_audio->nb_samples / 2;
                    } else {
                        auto audio_bytes = reinterpret_cast<uint8_t*>(&decoded_audio_bytes_[0]);
                        if (audio_bytes) {
                            src_audio->nb_samples     = remainaing_audio_samples_;
                            int bytes_left            = remainaing_audio_samples_ * 4 * src_audio->channels;
                            src_audio->data[0]        = audio_bytes + bytes_left;
                            src_audio->linesize[0]    = bytes_left;
                            remainaing_audio_samples_ = 0;
                        }
                    }
                }

                if (uhd_mode_ == 2 && conversion_buffer_.size() <= (width * height * 3)) {
                    // Do additional processing required to handle a 2SI input
                    memcpy(&conversion_buffer_[0], reserved_frames_.front()->image_data(), (width * height * 3));
                    blue_->convert_2si_to_sq(
                        width, height, &conversion_buffer_[0], reserved_frames_.front()->image_data());
                }

                // pass to caspar
                auto frame = core::draw_frame(make_frame(this, *frame_factory_, src_video, src_audio));
                if (!frame_buffer_.try_push(frame)) {
                    core::draw_frame dummy;
                    frame_buffer_.try_pop(dummy);
                    frame_buffer_.try_push(frame);
                    graph_->set_tag(diagnostics::tag_severity::WARNING, "dropped-frame");
                    CASPAR_LOG(warning) << print() << TEXT(" ERROR dropped frame.");
                }

                if (sync_format_ == UPD_FMT_FRAME || (sync_format_ == UPD_FMT_FIELD && !first_frame_))
                    boost::range::rotate(audio_cadence_, std::end(audio_cadence_) - 1);
            }
        } catch (...) {
            exception_ = std::current_exception();
            return E_FAIL;
        }

        return S_OK;
    }

    bool grab_frame_from_bluefishcard()
    {
        try {
            if (reserved_frames_.front()->image_data()) {
                if (sync_format_ == UPD_FMT_FIELD && first_frame_) {
                    blue_->system_buffer_read(const_cast<uint8_t*>(reserved_frames_.front()->image_data()),
                                              static_cast<unsigned long>(reserved_frames_.front()->image_size()),
                                              BlueImage_HANC_DMABuffer(dma_ready_captured_frame_id_, BLUE_DATA_FRAME),
                                              0);
                } else if (sync_format_ == UPD_FMT_FRAME) {
                    blue_->system_buffer_read(const_cast<uint8_t*>(reserved_frames_.front()->image_data()),
                                              static_cast<unsigned long>(reserved_frames_.front()->image_size()),
                                              BlueImage_HANC_DMABuffer(dma_ready_captured_frame_id_, BLUE_DATA_IMAGE),
                                              0);
                }
            } else {
                CASPAR_LOG(warning) << print() << TEXT(" NO image data in reserved frames list.");
                return false;
            }
            if (sync_format_ == UPD_FMT_FRAME || (sync_format_ == UPD_FMT_FIELD && !first_frame_)) {
                if (reserved_frames_.front()->hanc_data()) {
                    blue_->system_buffer_read(const_cast<uint8_t*>(reserved_frames_.front()->hanc_data()),
                                              static_cast<unsigned long>(reserved_frames_.front()->hanc_size()),
                                              BlueImage_HANC_DMABuffer(dma_ready_captured_frame_id_, BLUE_DATA_HANC),
                                              0);
                    //    CASPAR_LOG(warning) << print() << TEXT(" Hanc DMA Buf ID: ") << dma_ready_captured_frame_id_;
                }
            }
        } catch (...) {
            exception_ = std::current_exception();
        }
        return true;
    }

    void capture_thread_actual()
    {
        ULONG        current_field_count        = 0;
        ULONG        last_field_count           = 0;
        ULONG        start_field_count          = 0;
        unsigned int current_input_video_signal = VID_FMT_EXT_INVALID;

        last_field_count  = current_field_count;
        start_field_count = current_field_count;

        blue_->wait_video_input_sync(UPD_FMT_FRAME, &current_field_count);
        while (process_capture_) {
            // tell the card to capture another frame at the next interrupt
            schedule_capture();
            blue_->wait_video_input_sync((EUpdateMethod)sync_format_, &current_field_count);
            get_capture_time();

            if (last_field_count + 3 < current_field_count)
                CASPAR_LOG(warning) << L"Error: dropped " << (current_field_count - last_field_count - 2) / 2
                                    << L" frames" << L"Current " << current_field_count << L"  Old "
                                    << last_field_count;

            last_field_count = current_field_count;
            blue_->get_card_property32(VIDEO_MODE_EXT_INPUT, &current_input_video_signal);
            if (current_input_video_signal != VID_FMT_EXT_INVALID &&
                dma_ready_captured_frame_id_ != std::numeric_limits<ULONG>::max()) {
                // DoneID is now what ScheduleID was at the last iteration when we called
                // render_buffer_capture(ScheduleID) we just checked if the video signal for the buffer “DoneID” was
                // valid while it was capturing so we can DMA the buffer DMA the frame from the card to our buffer
                if (grab_frame_from_bluefishcard())
                    process_data();
                processing_benchmark_timer_.restart();
            }
            if (sync_format_ == UPD_FMT_FRAME || (sync_format_ == UPD_FMT_FIELD && !first_frame_))
                boost::range::rotate(reserved_frames_, std::begin(reserved_frames_) + 1);

            frames_captured++;

            if (sync_format_ == UPD_FMT_FIELD)
                first_frame_ = !first_frame_;
        }
    }

    ~bluefish_producer()
    {
        try {
            process_capture_ = false;
            if (capture_thread_) {
                Sleep(41);
                capture_thread_->join();
            }

            blue_->detach();
        } catch (...) {
            CASPAR_LOG_CURRENT_EXCEPTION();
        }
    }

    core::draw_frame get_frame()
    {
        if (exception_ != nullptr)
            std::rethrow_exception(exception_);

        core::draw_frame frame;
        if (!frame_buffer_.try_pop(frame)) {
            graph_->set_tag(diagnostics::tag_severity::WARNING, "late-frame");
        }
        return frame;
    }

    std::wstring print() const
    {
        return model_name_ + L" [" + std::to_wstring(device_index_) + L"|" + format_desc_.name + L"]";
    }

    core::monitor::state state() const
    {
        std::lock_guard<std::mutex> lock(state_mutex_);
        return state_;
    }

    boost::rational<int> get_out_framerate() const { return format_desc_.framerate; }
};

class bluefish_producer_proxy : public core::frame_producer
{
    std::unique_ptr<bluefish_producer> producer_;
    const uint32_t                     length_;
    executor                           executor_;

  public:
    explicit bluefish_producer_proxy(const core::video_format_desc&              format_desc,
                                     const spl::shared_ptr<core::frame_factory>& frame_factory,
                                     int                                         device_index,
                                     int                                         stream_index,
                                     int                                         uhd_mode,
                                     uint32_t                                    length)
        : length_(length)
        , executor_(L"bluefish_producer[" + std::to_wstring(device_index) + L"]")
    {
        auto ctx = core::diagnostics::call_context::for_thread();
        executor_.invoke([=] {
            core::diagnostics::call_context::for_thread() = ctx;
            producer_.reset(new bluefish_producer(format_desc, device_index, stream_index, uhd_mode, frame_factory));
        });
    }

    ~bluefish_producer_proxy()
    {
        executor_.invoke([=] { producer_.reset(); });
    }

    core::monitor::state state() const override { return producer_->state(); }

    // frame_producer

    core::draw_frame receive_impl(int nb_samples) override { return producer_->get_frame(); }

    uint32_t nb_frames() const override { return length_; }

    std::wstring print() const override { return producer_->print(); }

    std::wstring name() const override { return L"bluefish"; }

    boost::rational<int> get_out_framerate() const { return producer_->get_out_framerate(); }
};

spl::shared_ptr<core::frame_producer> create_producer(const core::frame_producer_dependencies& dependencies,
                                                      const std::vector<std::wstring>&         params)
{
    if (params.empty() || !boost::iequals(params.at(0), "bluefish"))
        return core::frame_producer::empty();

    auto device_index = get_param(L"DEVICE", params, -1);
    if (device_index == -1)
        device_index = std::stoi(params.at(1));

    auto stream_index = get_param(L"SDI-STREAM", params, -1);
    if (stream_index == -1)
        stream_index = 1;

    auto uhd_mode = get_param(L"UHD-MODE", params, -1);
    if (uhd_mode == -1)
        uhd_mode = 0;

    auto length         = get_param(L"LENGTH", params, std::numeric_limits<uint32_t>::max());
    auto in_format_desc = core::video_format_desc(get_param(L"FORMAT", params, L"INVALID"));

    auto producer = spl::make_shared<bluefish_producer_proxy>(
        dependencies.format_desc, dependencies.frame_factory, device_index, stream_index, uhd_mode, length);

    return create_destroy_proxy(producer);
}
}} // namespace caspar::bluefish
