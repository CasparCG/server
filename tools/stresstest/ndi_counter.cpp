// NDI output probe for the CasparCG stress test.
//
// Connects to an NDI source, decodes the 16 bit frame counter burned into the test clips
// (16 black/white blocks of 40x40 px at 1080p, top-left corner, scaled with the frame width)
// and reports frame pacing as JSON:
//   fps, late frames (sender interval > 1.5 x nominal), duplicated / skipped counter values.
//
// usage: ndi_counter <libndi.dylib> <source name substring> <seconds> [clip length in frames]
//
// The NDI library is loaded at runtime (like CasparCG does), so only the headers shipped in
// src/modules/newtek/interop are needed to build this.

#include <cstddef>
#include <cstdint>

#include <Processing.NDI.Lib.h>

#include <dlfcn.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    setvbuf(stdout, nullptr, _IONBF, 0);
    if (argc < 4) {
        std::fprintf(stderr, "usage: %s <libndi.dylib> <source> <seconds> [clip frames]\n", argv[0]);
        return 1;
    }
    const char* lib_path = argv[1];
    const char* match    = argv[2];
    const int   seconds  = std::atoi(argv[3]);
    const int   clip     = argc > 4 ? std::atoi(argv[4]) : 0;

    void* lib = dlopen(lib_path, RTLD_LOCAL | RTLD_LAZY);
    if (!lib) {
        std::printf("{\"error\": \"cannot load %s\"}\n", lib_path);
        return 1;
    }
    auto load = reinterpret_cast<const NDIlib_v6* (*)()>(dlsym(lib, "NDIlib_v6_load"));
    const NDIlib_v6* ndi = load ? load() : nullptr;
    if (!ndi || !ndi->initialize()) {
        std::printf("{\"error\": \"cannot initialize NDI\"}\n");
        return 1;
    }

    NDIlib_find_create_t find_settings{};
    find_settings.show_local_sources = true;
    auto        finder               = ndi->find_create_v2(&find_settings);
    std::string name;
    for (int i = 0; i < 15 && name.empty(); ++i) {
        ndi->find_wait_for_sources(finder, 1000);
        uint32_t count   = 0;
        auto     sources = ndi->find_get_current_sources(finder, &count);
        for (uint32_t n = 0; n < count; ++n) {
            if (std::strstr(sources[n].p_ndi_name, match)) {
                name = sources[n].p_ndi_name;
            }
        }
    }
    if (name.empty()) {
        std::printf("{\"error\": \"NDI source '%s' not found\"}\n", match);
        return 2;
    }

    NDIlib_source_t source{};
    source.p_ndi_name = name.c_str();
    NDIlib_recv_create_v3_t recv_settings{};
    recv_settings.source_to_connect_to = source;
    recv_settings.color_format         = NDIlib_recv_color_format_BGRX_BGRA;
    recv_settings.bandwidth            = NDIlib_recv_bandwidth_highest;
    auto recv                          = ndi->recv_create_v3(&recv_settings);

    int     frames = 0, sequential = 0, duplicated = 0, skipped = 0, missing = 0, loops = 0, late = 0;
    int     prev = -1, width = 0, height = 0;
    double  nominal_ms = 20.0, max_interval = 0.0;
    int64_t first_ts = -1, last_ts = -1;
    std::vector<double> intervals;

    // Let the connection settle before measuring
    const auto settle = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    const auto end    = settle + std::chrono::seconds(seconds);

    while (std::chrono::steady_clock::now() < end) {
        NDIlib_video_frame_v2_t video;
        NDIlib_audio_frame_v2_t audio;
        auto type = ndi->recv_capture_v2(recv, &video, &audio, nullptr, 500);
        if (type == NDIlib_frame_type_audio) {
            ndi->recv_free_audio_v2(recv, &audio);
            continue;
        }
        if (type != NDIlib_frame_type_video) {
            continue;
        }
        if (std::chrono::steady_clock::now() < settle) {
            ndi->recv_free_video_v2(recv, &video);
            continue;
        }

        width      = video.xres;
        height     = video.yres;
        nominal_ms = 1000.0 * video.frame_rate_D / video.frame_rate_N;

        const double scale = video.xres / 1920.0;
        int          code  = 0;
        for (int bit = 0; bit < 16; ++bit) {
            const int      x = static_cast<int>((20 + 40 * bit) * scale);
            const int      y = static_cast<int>(20 * scale);
            const uint8_t* p = video.p_data + y * video.line_stride_in_bytes + x * 4;
            if (p[1] > 128) {
                code |= 1 << bit;
            }
        }

        if (last_ts >= 0) {
            const double interval = (video.timestamp - last_ts) / 10000.0;
            intervals.push_back(interval);
            max_interval = std::max(max_interval, interval);
            if (interval > nominal_ms * 1.5) {
                late++;
            }
        }
        if (first_ts < 0) {
            first_ts = video.timestamp;
        }
        last_ts = video.timestamp;

        if (prev >= 0) {
            const int delta = code - prev;
            if (delta == 1) {
                sequential++;
            } else if (delta == 0) {
                duplicated++;
            } else if (delta > 1) {
                skipped++;
                missing += delta - 1;
            } else if (clip > 0 && prev == clip - 1 && code == 0) {
                loops++;
            } else {
                skipped++; // jump backwards that is not a clean loop
            }
        }
        prev = code;
        frames++;
        ndi->recv_free_video_v2(recv, &video);
    }

    double mean = 0, sd = 0;
    for (double x : intervals) {
        mean += x;
    }
    if (!intervals.empty()) {
        mean /= intervals.size();
        for (double x : intervals) {
            sd += (x - mean) * (x - mean);
        }
        sd = std::sqrt(sd / intervals.size());
    }
    // Frame rate from the sender's timestamps (independent of network/receiver jitter)
    const double span = (last_ts - first_ts) / 1e7;
    const double fps  = (frames > 1 && span > 0) ? (frames - 1) / span : 0.0;

    NDIlib_recv_performance_t total{}, dropped{};
    ndi->recv_get_performance(recv, &total, &dropped);

    std::printf("{\"source\": \"%s\", \"width\": %d, \"height\": %d, \"frames\": %d, \"fps\": %.2f, "
                "\"nominal_ms\": %.2f, \"interval_mean_ms\": %.2f, \"interval_sd_ms\": %.2f, \"interval_max_ms\": %.2f, "
                "\"late\": %d, \"sequential\": %d, \"duplicated\": %d, \"skipped\": %d, \"missing\": %d, \"loops\": %d, "
                "\"receiver_dropped\": %lld}\n",
                name.c_str(), width, height, frames, fps, nominal_ms, mean, sd, max_interval, late, sequential,
                duplicated, skipped, missing, loops, static_cast<long long>(dropped.video_frames));

    ndi->recv_destroy(recv);
    ndi->find_destroy(finder);
    ndi->destroy();
    return 0;
}
