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
 * Author: Niklas Andersson, niklas@niklaspandersson.se
 */

#include "vulkan_frame_import.h"

#include "../ffmpeg.h"
#include "../util/av_util.h"

#include <accelerator/vulkan/util/barrier.h>
#include <accelerator/vulkan/util/command_context.h>
#include <accelerator/vulkan/util/device.h>
#include <accelerator/vulkan/util/queue_manager.h>
#include <accelerator/vulkan/util/texture.h>
#include <accelerator/vulkan/util/vulkan_queue.h>

#include <boost/property_tree/ptree.hpp>

#include <common/env.h>
#include <common/log.h>
#include <common/memory.h>
#include <common/scope_exit.h>

#include <core/frame/frame_factory.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/hwcontext.h>
#include <libavutil/hwcontext_vulkan.h>
#include <libavutil/pixdesc.h>
}

#include <VkBootstrap.h>
#include <vulkan/vulkan.hpp>

#include <algorithm>
#include <cstring>
#include <mutex>
#include <optional>
#include <utility>

namespace caspar { namespace ffmpeg {

namespace {

using namespace caspar::accelerator::vulkan;

/// The queue the decode->mixer copy runs on. A dedicated transfer family keeps the blit off
/// the render queue; on hardware without one, queue_manager aliases this to the render queue
/// and every hand-off below collapses to a plain barrier.
constexpr queue_type import_queue = queue_type::transfer;

/// Everything the AVHWDeviceContext points at but does not own. FFmpeg keeps these pointers
/// for the context's lifetime, so this hangs off user_opaque and dies with the context. The
/// device reference is what keeps the extension name strings it points into alive.
struct hw_device_binding
{
    std::shared_ptr<accelerator::vulkan::device> device;
    std::vector<const char*>                     instance_extensions;
    std::vector<const char*>                     device_extensions;
};

std::mutex                   g_mutex;
std::weak_ptr<device>        g_accelerator_device;
std::shared_ptr<AVBufferRef> g_hw_device_ctx;
bool                         g_hw_device_failed = false;

/// FFmpeg submits its decode work on our queues, and vkQueueSubmit needs the queue
/// externally synchronized. These bracket its submissions with the same lock our own
/// submitters take, so the two can never race on one VkQueue.
void lock_queue(AVHWDeviceContext* ctx, uint32_t queue_family, uint32_t index)
{
    auto* binding = static_cast<hw_device_binding*>(ctx->user_opaque);
    if (auto queue = binding->device->queue_at(queue_family, index))
        queue->lock();
}

void unlock_queue(AVHWDeviceContext* ctx, uint32_t queue_family, uint32_t index)
{
    auto* binding = static_cast<hw_device_binding*>(ctx->user_opaque);
    if (auto queue = binding->device->queue_at(queue_family, index))
        queue->unlock();
}

std::shared_ptr<AVBufferRef> create_hw_device_ctx(const std::shared_ptr<device>& dev)
{
    auto* ref = av_hwdevice_ctx_alloc(AV_HWDEVICE_TYPE_VULKAN);
    if (!ref)
        return nullptr;

    auto  handle     = std::shared_ptr<AVBufferRef>(ref, [](AVBufferRef* p) { av_buffer_unref(&p); });
    auto* device_ctx = reinterpret_cast<AVHWDeviceContext*>(ref->data);
    auto* vk_ctx     = static_cast<AVVulkanDeviceContext*>(device_ctx->hwctx);

    auto binding    = std::make_unique<hw_device_binding>();
    binding->device = dev;
    for (const auto& ext : dev->enabled_instance_extensions())
        binding->instance_extensions.push_back(ext.c_str());
    for (const auto& ext : dev->enabled_device_extensions())
        binding->device_extensions.push_back(ext.c_str());

    // FFmpeg derives which of its code paths are usable purely from these lists, so they have
    // to be what we actually enabled — the video decode extensions included.
    vk_ctx->get_proc_addr              = dev->instance_proc_addr();
    vk_ctx->inst                       = dev->instance();
    vk_ctx->phys_dev                   = dev->physical_device();
    vk_ctx->act_dev                    = dev->getVkDevice();
    vk_ctx->enabled_inst_extensions    = binding->instance_extensions.data();
    vk_ctx->nb_enabled_inst_extensions = static_cast<int>(binding->instance_extensions.size());
    vk_ctx->enabled_dev_extensions     = binding->device_extensions.data();
    vk_ctx->nb_enabled_dev_extensions  = static_cast<int>(binding->device_extensions.size());

    // The queues FFmpeg may submit on. Listing more than one family also makes the frames it
    // allocates VK_SHARING_MODE_CONCURRENT across all of them, which is what lets the import
    // below read a decoded image from a different queue with a plain barrier instead of a
    // queue-family ownership transfer FFmpeg would never record the release half of.
    int nb_qf = 0;
    for (const auto& family : dev->queue_families()) {
        if (nb_qf >= static_cast<int>(FF_ARRAY_ELEMS(vk_ctx->qf)))
            break;
        vk_ctx->qf[nb_qf].idx   = static_cast<int>(family.index);
        vk_ctx->qf[nb_qf].num   = static_cast<int>(family.count);
        vk_ctx->qf[nb_qf].flags = static_cast<VkQueueFlagBits>(static_cast<VkQueueFlags>(family.usage));
        // Left zero on purpose: av_hwdevice_ctx_init fills in the codec operations for any
        // family we flagged as a video one.
        vk_ctx->qf[nb_qf].video_caps = static_cast<VkVideoCodecOperationFlagBitsKHR>(0);
        ++nb_qf;
    }
    vk_ctx->nb_qf = nb_qf;

    device_ctx->user_opaque = binding.release();
    device_ctx->free        = [](AVHWDeviceContext* ctx) { delete static_cast<hw_device_binding*>(ctx->user_opaque); };

    vk_ctx->lock_queue   = lock_queue;
    vk_ctx->unlock_queue = unlock_queue;

    const auto ret = av_hwdevice_ctx_init(ref);
    if (ret < 0) {
        CASPAR_LOG(warning) << L"[ffmpeg] Could not share the Vulkan accelerator device with FFmpeg; "
                               L"hardware decoding disabled.";
        return nullptr;
    }

    return handle;
}

/// The process-wide hardware device context, built on first use. A failure is remembered so
/// every later producer takes the CPU strategy without retrying (and re-logging).
std::shared_ptr<AVBufferRef> hw_device_ctx()
{
    std::lock_guard<std::mutex> lock(g_mutex);

    if (g_hw_device_ctx || g_hw_device_failed)
        return g_hw_device_ctx;

    auto dev = g_accelerator_device.lock();
    if (!dev) {
        CASPAR_LOG(debug) << L"[ffmpeg] No Vulkan accelerator device to decode on; hardware decoding disabled.";
        g_hw_device_failed = true;
        return nullptr;
    }

    g_hw_device_ctx    = create_hw_device_ctx(dev);
    g_hw_device_failed = !g_hw_device_ctx;
    if (g_hw_device_ctx)
        CASPAR_LOG(info) << L"[ffmpeg] Vulkan hardware decoding available on the accelerator device.";

    return g_hw_device_ctx;
}

/// FFmpeg hands a frame over as either one image per plane or a single multi-planar image;
/// this is the mapping it uses itself (libavutil/vulkan.c, ff_vk_aspect_flag).
int image_index_of_plane(int plane, int nb_images) { return std::min(plane, nb_images - 1); }

vk::ImageAspectFlags aspect_of_plane(int plane, int nb_planes, int nb_images)
{
    if (nb_planes == nb_images)
        return vk::ImageAspectFlagBits::eColor;

    static const vk::ImageAspectFlagBits plane_aspect[] = {
        vk::ImageAspectFlagBits::ePlane0, vk::ImageAspectFlagBits::ePlane1, vk::ImageAspectFlagBits::ePlane2};
    return plane_aspect[plane];
}

/// The producer's 16-channel audio layout, matching what make_frame() builds for host frames.
array<std::int32_t> to_audio_array(const std::shared_ptr<AVFrame>& audio)
{
    if (!audio)
        return {};

    const int channel_count = 16;
    auto      data          = std::vector<std::int32_t>(audio->nb_samples * channel_count, 0);

    const auto  source_channel_count = audio->ch_layout.nb_channels;
    const auto* src                  = reinterpret_cast<const std::int32_t*>(audio->data[0]);

    if (source_channel_count == channel_count) {
        std::memcpy(data.data(), src, sizeof(std::int32_t) * channel_count * audio->nb_samples);
    } else {
        for (auto i = 0; i < audio->nb_samples; i++) {
            for (auto j = 0; j < std::min(channel_count, source_channel_count); ++j) {
                data[i * channel_count + j] = src[i * source_channel_count + j];
            }
        }
    }

    return array<std::int32_t>(std::move(data));
}

/// The mixer's plane layout for a decoder's software format, or nothing when it has none. The
/// mixer samples one texture per plane, so a format it cannot describe — or one whose description
/// disagrees with the plane count the decoder will actually hand over — cannot be imported.
std::optional<core::pixel_format_desc> mixer_layout_of(AVPixelFormat     sw_format,
                                                       int               width,
                                                       int               height,
                                                       core::color_space color_space = core::color_space::bt709)
{
    std::vector<int> data_map;
    auto             desc = pixel_format_desc(sw_format, width, height, data_map, color_space);

    if (desc.format == core::pixel_format::invalid ||
        static_cast<int>(desc.planes.size()) != av_pix_fmt_count_planes(sw_format)) {
        return std::nullopt;
    }
    return desc;
}

} // namespace

// ---------------------------------------------------------------------------------------

struct vulkan_frame_import::impl
{
    std::shared_ptr<core::frame_factory>         frame_factory;
    std::shared_ptr<accelerator::vulkan::device> accelerator_device;
    std::shared_ptr<AVBufferRef>                 device; // null unless FFmpeg has to know our device
    gpu_producer                                 gpu;
    bool                                         warned_unsupported_format = false;

    /// Sources that have to outlive their copy. A frame whose surface belongs to another API with
    /// no timeline of its own (VideoToolbox) is held here until the submit that read it has
    /// retired; FFmpeg's Vulkan frames need nothing, because their semaphore says it.
    struct in_flight
    {
        completion_token      token;
        std::shared_ptr<void> source;
    };
    std::vector<in_flight> in_flight;

    /// Drop everything the GPU is done with. Cheap and non-blocking: a timeline query per entry.
    void retire_completed()
    {
        const auto done = [&](const struct in_flight& f) { return gpu.context().wait(f.token, 0); };
        in_flight.erase(std::remove_if(in_flight.begin(), in_flight.end(), done), in_flight.end());
    }
};

vulkan_frame_import::vulkan_frame_import(std::unique_ptr<impl> i)
    : impl_(std::move(i))
{
}

vulkan_frame_import::~vulkan_frame_import() = default;

namespace {

/// The accelerator's Vulkan device, or null when this process has none.
std::shared_ptr<device> accelerator_device()
{
    std::lock_guard<std::mutex> lock(g_mutex);
    return g_accelerator_device.lock();
}

} // namespace

std::unique_ptr<vulkan_frame_import>
vulkan_frame_import::create(const std::shared_ptr<core::frame_factory>& frame_factory)
{
    auto import = create_for_import(frame_factory);
    if (!import)
        return nullptr;

    // The half that only a decoder writing into our device needs: FFmpeg has to be handed the
    // device, the queues and the extension lists before it can target it.
    auto device = hw_device_ctx();
    if (!device)
        return nullptr;

    import->impl_->device = std::move(device);
    return import;
}

std::unique_ptr<vulkan_frame_import>
vulkan_frame_import::create_for_import(const std::shared_ptr<core::frame_factory>& frame_factory)
{
    if (!frame_factory)
        return nullptr;

    // Only the Vulkan accelerator implements the GPU producer path; on any other channel this
    // reports false and the caller takes the CPU strategy.
    gpu_producer gpu{spl::make_shared_ptr(frame_factory), import_queue};
    if (!gpu) {
        CASPAR_LOG(debug) << L"[ffmpeg] This channel is not on the Vulkan accelerator; hardware decoding disabled.";
        return nullptr;
    }

    auto dev = accelerator_device();
    if (!dev) {
        CASPAR_LOG(debug) << L"[ffmpeg] No Vulkan accelerator device to decode on; hardware decoding disabled.";
        return nullptr;
    }

    auto i                = std::make_unique<impl>();
    i->frame_factory      = frame_factory;
    i->accelerator_device = std::move(dev);
    i->gpu                = std::move(gpu);
    return std::unique_ptr<vulkan_frame_import>(new vulkan_frame_import(std::move(i)));
}

AVBufferRef* vulkan_frame_import::device() const { return impl_->device ? impl_->device.get() : nullptr; }

bool vulkan_frame_import::has_device_extension(const char* name) const
{
    for (const auto& extension : impl_->accelerator_device->enabled_device_extensions()) {
        if (extension == name)
            return true;
    }
    return false;
}

vk::Device vulkan_frame_import::vk_device() const { return impl_->accelerator_device->getVkDevice(); }

void vulkan_frame_import::drain()
{
    auto& ctx = impl_->gpu.context();
    ctx.wait(ctx.current_completion(), UINT64_MAX);
    impl_->in_flight.clear();
}

bool vulkan_frame_import::has_mixer_layout(AVPixelFormat sw_format, int width, int height)
{
    return mixer_layout_of(sw_format, width, height).has_value();
}

std::optional<core::pixel_format_desc>
vulkan_frame_import::mixer_layout(AVPixelFormat sw_format, int width, int height, core::color_space color_space)
{
    return mixer_layout_of(sw_format, width, height, color_space);
}

core::draw_frame vulkan_frame_import::host_frame(void*                            tag,
                                                 std::shared_ptr<AVFrame>         video,
                                                 std::shared_ptr<AVFrame>         audio,
                                                 core::color_space                color_space,
                                                 core::frame_geometry::scale_mode scale_mode)
{
    return core::draw_frame(
        ffmpeg::make_frame(tag, *impl_->frame_factory, std::move(video), std::move(audio), color_space, scale_mode));
}

core::draw_frame vulkan_frame_import::import(void*                            tag,
                                             const std::shared_ptr<AVFrame>&  video,
                                             const std::shared_ptr<AVFrame>&  audio,
                                             core::color_space                color_space,
                                             core::frame_geometry::scale_mode scale_mode)
{
    auto* vkf        = reinterpret_cast<AVVkFrame*>(video->data[0]);
    auto* frames_ctx = reinterpret_cast<AVHWFramesContext*>(video->hw_frames_ctx->data);
    auto* vk_frames  = static_cast<AVVulkanFramesContext*>(frames_ctx->hwctx);

    // video_strategy::accepts() already refused any format without a layout, so this only guards
    // a decoder that changed format mid-stream.
    auto layout = mixer_layout_of(frames_ctx->sw_format, video->width, video->height, color_space);
    if (!layout) {
        if (!impl_->warned_unsupported_format) {
            impl_->warned_unsupported_format = true;
            CASPAR_LOG(warning) << L"[ffmpeg] Hardware frame format changed to "
                                << u16(av_get_pix_fmt_name(frames_ctx->sw_format))
                                << L", which the mixer has no layout for; dropping video.";
        }
        return core::draw_frame{};
    }

    int nb_images = 0;
    while (nb_images < AV_NUM_DATA_POINTERS && vkf->img[nb_images])
        ++nb_images;

    // FFmpeg's contract: read the frame's properties under its lock, wait its timeline at
    // the value it reports, signal that timeline back at an incremented value, and record
    // the new layout — the signal is how FFmpeg learns the surface may be recycled.
    vk_frames->lock_frame(frames_ctx, vkf);
    CASPAR_SCOPE_EXIT { vk_frames->unlock_frame(frames_ctx, vkf); };

    std::vector<vk::Image>       src_images(nb_images);
    std::vector<vk::ImageLayout> src_layouts(nb_images);
    external_sync                sync;
    for (int i = 0; i < nb_images; ++i) {
        src_images[i]  = vk::Image(vkf->img[i]);
        src_layouts[i] = static_cast<vk::ImageLayout>(vkf->layout[i]);
        sync.wait.push_back({vk::Semaphore(vkf->sem[i]), vkf->sem_value[i]});
        sync.signal.push_back({vk::Semaphore(vkf->sem[i]), vkf->sem_value[i] + 1});
    }

    auto frame = import_images(tag, src_images, src_layouts, *layout, sync, audio, scale_mode);

    for (int i = 0; i < nb_images; ++i) {
        vkf->sem_value[i] += 1;
        vkf->layout[i] = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
        vkf->access[i] = VK_ACCESS_TRANSFER_READ_BIT;
    }

    return frame;
}

core::draw_frame vulkan_frame_import::import_images(void*                                     tag,
                                                    const std::vector<vk::Image>&             images,
                                                    const std::vector<vk::ImageLayout>&       layouts,
                                                    const core::pixel_format_desc&            desc,
                                                    const accelerator::vulkan::external_sync& sync,
                                                    const std::shared_ptr<AVFrame>&           audio,
                                                    core::frame_geometry::scale_mode          scale_mode,
                                                    std::shared_ptr<void>                     keep_alive)
{
    const int nb_planes = static_cast<int>(desc.planes.size());
    const int nb_images = static_cast<int>(images.size());

    // One mixer texture per plane, sized and strided exactly as the desc the mixer will
    // read it back with, so the copy extents and the shader agree by construction.
    std::vector<producer_plane> planes;
    planes.reserve(desc.planes.size());
    for (const auto& plane : desc.planes) {
        producer_plane p;
        p.tex = impl_->gpu.factory().create_producer_texture(plane.width, plane.height, plane.stride, plane.depth);
        p.from_layout = vk::ImageLayout::eUndefined; // pooled texture, previous contents discardable
        p.work_layout = vk::ImageLayout::eTransferDstOptimal;
        p.work_stage  = vk::PipelineStageFlagBits2::eTransfer;
        p.work_access = vk::AccessFlagBits2::eTransferWrite;
        planes.push_back(std::move(p));
    }

    auto record = [&](vk::CommandBuffer cmd, const std::vector<std::shared_ptr<texture>>& textures) {
        // Move the source images to a transfer source. FFmpeg's are CONCURRENT across every queue
        // family we registered with the hardware device context, so this is a plain transition: an
        // ownership transfer is neither needed nor legal on such an image, and the caller's
        // timeline wait is what orders it after the decode. An image imported from another API
        // arrives in eUndefined, which discards contents on paper but not on the driver that has
        // one — MoltenVK, where layouts are near no-ops and the memory is the IOSurface itself.
        for (int i = 0; i < nb_images; ++i) {
            transitionImageLayout(images[i],
                                  layouts[i],
                                  vk::AccessFlagBits2::eNone,
                                  vk::PipelineStageFlagBits2::eTopOfPipe,
                                  vk::ImageLayout::eTransferSrcOptimal,
                                  vk::AccessFlagBits2::eTransferRead,
                                  vk::PipelineStageFlagBits2::eTransfer,
                                  cmd);
        }

        for (int n = 0; n < nb_planes; ++n) {
            const auto& dst = textures[n];

            vk::ImageCopy region;
            region.srcSubresource = vk::ImageSubresourceLayers(aspect_of_plane(n, nb_planes, nb_images), 0, 0, 1);
            region.dstSubresource = vk::ImageSubresourceLayers(vk::ImageAspectFlagBits::eColor, 0, 0, 1);
            region.extent = vk::Extent3D(static_cast<uint32_t>(dst->width()), static_cast<uint32_t>(dst->height()), 1);

            cmd.copyImage(images[image_index_of_plane(n, nb_images)],
                          vk::ImageLayout::eTransferSrcOptimal,
                          vk::Image(dst->id()),
                          vk::ImageLayout::eTransferDstOptimal,
                          region);
        }
    };

    auto geometry = scale_mode != core::frame_geometry::scale_mode::stretch
                        ? core::frame_geometry::get_default(scale_mode)
                        : core::frame_geometry::get_default();

    auto frame =
        impl_->gpu.produce(tag, std::move(planes), desc, record, to_audio_array(audio), sync, std::move(geometry));

    if (keep_alive) {
        impl_->retire_completed();
        // The context is shared, so this token may belong to a later submit than ours. Holding the
        // source a little longer than strictly necessary is the safe direction to err in.
        impl_->in_flight.push_back({impl_->gpu.context().current_completion(), std::move(keep_alive)});
    }

    return core::draw_frame(std::move(frame));
}

void register_vulkan_requirements(vkb::PhysicalDevice& pd)
{
    // The decode side of Vulkan video. Each is optional — a device without them simply has no
    // hardware decoding and every producer falls back to the CPU strategy.
    pd.enable_extension_if_present(VK_KHR_VIDEO_QUEUE_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_VIDEO_DECODE_QUEUE_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_VIDEO_DECODE_H264_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_VIDEO_DECODE_H265_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_VIDEO_DECODE_AV1_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_VIDEO_MAINTENANCE_1_EXTENSION_NAME);

    // External memory and semaphores, which is how another API is handed a Vulkan frame. Needed
    // by the CUDA path (FFmpeg exports our frame's memory to CUDA and copies NVDEC's output into
    // it, synchronised through the frame's own timeline semaphores), and harmless otherwise.
    pd.enable_extension_if_present(VK_KHR_EXTERNAL_MEMORY_FD_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_EXTERNAL_SEMAPHORE_FD_EXTENSION_NAME);
#ifdef _WIN32
    pd.enable_extension_if_present(VK_KHR_EXTERNAL_MEMORY_WIN32_EXTENSION_NAME);
    pd.enable_extension_if_present(VK_KHR_EXTERNAL_SEMAPHORE_WIN32_EXTENSION_NAME);
#endif
#ifdef __APPLE__
    // How a Metal texture comes to back a VkImage, which is the whole VideoToolbox path: its
    // frames are IOSurfaces, and this is the only route MoltenVK offers into one. Named as a
    // literal to keep the Metal platform headers out of this file.
    pd.enable_extension_if_present("VK_EXT_metal_objects");
#endif

    // FFmpeg's decoder needs the feature, not just the extension: it allocates DPB images
    // without a video profile.
    vk::PhysicalDeviceVideoMaintenance1FeaturesKHR video_maintenance1;
    video_maintenance1.videoMaintenance1 = true;
    pd.enable_extension_features_if_present(video_maintenance1);

    // FFmpeg's own shaders (the Vulkan filters, and the decoder's software-defined paths) need
    // 64-bit integers and sampled-image gather.
    vk::PhysicalDeviceFeatures2 features;
    features.features.shaderInt64               = true;
    features.features.shaderImageGatherExtended = true;
    pd.enable_features_if_present(features.features);
}

void set_vulkan_accelerator_device(const std::shared_ptr<accelerator::accelerator_device>& accelerator_device)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    g_accelerator_device = std::dynamic_pointer_cast<device>(accelerator_device);
    // The cached context holds a strong device reference; drop it so the device isn't
    // destroyed by static destructors after the driver has shut down.
    g_hw_device_ctx.reset();
    g_hw_device_failed = false;
}

}} // namespace caspar::ffmpeg
