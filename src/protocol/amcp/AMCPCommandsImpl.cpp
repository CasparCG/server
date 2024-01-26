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
 * Author: Nicklas P Andersson
 */

#include "../StdAfx.h"

#if defined(_MSC_VER)
#pragma warning(push, 1) // TODO: Legacy code, just disable warnings
#endif

#include "AMCPCommandsImpl.h"

#include "AMCPCommandQueue.h"
#include "amcp_command_repository.h"

#include <common/env.h>

#include <common/base64.h>
#include <common/filesystem.h>
#include <common/future.h>
#include <common/log.h>
#include <common/os/filesystem.h>
#include <common/param.h>

#include <core/consumer/output.h>
#include <core/diagnostics/call_context.h>
#include <core/diagnostics/osd_graph.h>
#include <core/frame/frame_transform.h>
#include <core/mixer/mixer.h>
#include <core/producer/cg_proxy.h>
#include <core/producer/color/color_producer.h>
#include <core/producer/frame_producer.h>
#include <core/producer/stage.h>
#include <core/producer/transition/sting_producer.h>
#include <core/producer/transition/transition_producer.h>
#include <core/video_format.h>

#include <algorithm>
#include <fstream>
#include <future>
#include <memory>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/regex.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/locale.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/regex.hpp>

#include <tbb/concurrent_unordered_map.h>

/* Return codes

102 [action]			Information that [action] has happened
101 [action]			Information that [action] has happened plus one row of data

202 [command] OK		[command] has been executed
201 [command] OK		[command] has been executed, plus one row of data
200 [command] OK		[command] has been executed, plus multiple lines of data. ends with an empty line

400 ERROR				the command could not be understood
401 [command] ERROR		invalid/missing channel
402 [command] ERROR		parameter missing
403 [command] ERROR		invalid parameter
404 [command] ERROR		file not found

500 FAILED						internal error
501 [command] FAILED			internal error
502 [command] FAILED			could not read file
503 [command] FAILED			access denied
504 [command] QUEUE OVERFLOW	command queue overflow

600 [command] FAILED	[command] not implemented
*/

namespace caspar { namespace protocol { namespace amcp {

using namespace core;
namespace pt = boost::property_tree;

std::wstring read_utf8_file(const boost::filesystem::path& file)
{
    std::wstringstream           result;
    boost::filesystem::wifstream filestream(file);

    if (filestream) {
        // Consume BOM first
        filestream.get();
        // read all data
        result << filestream.rdbuf();
    }

    return result.str();
}

std::wstring read_latin1_file(const boost::filesystem::path& file)
{
    boost::locale::generator gen;
    gen.locale_cache_enabled(true);
#if BOOST_VERSION >= 108100
    gen.categories(boost::locale::category_t::codepage);
#else
    gen.categories(boost::locale::codepage_facet);
#endif

    std::stringstream           result_stream;
    boost::filesystem::ifstream filestream(file);
    filestream.imbue(gen("en_US.ISO8859-1"));

    if (filestream) {
        // read all data
        result_stream << filestream.rdbuf();
    }

    std::string  result = result_stream.str();
    std::wstring widened_result;

    // The first 255 codepoints in unicode is the same as in latin1
    boost::copy(result | boost::adaptors::transformed([](char c) { return static_cast<unsigned char>(c); }),
                std::back_inserter(widened_result));

    return widened_result;
}

std::wstring read_file(const boost::filesystem::path& file)
{
    static const uint8_t BOM[] = {0xef, 0xbb, 0xbf};

    if (!boost::filesystem::exists(file)) {
        return L"";
    }

    if (boost::filesystem::file_size(file) >= 3) {
        boost::filesystem::ifstream bom_stream(file);

        char header[3];
        bom_stream.read(header, 3);
        bom_stream.close();

        if (std::memcmp(BOM, header, 3) == 0)
            return read_utf8_file(file);
    }

    return read_latin1_file(file);
}

std::vector<spl::shared_ptr<core::video_channel>> get_channels(const command_context& ctx)
{
    std::vector<spl::shared_ptr<core::video_channel>> result;
    for (auto& cc : *ctx.channels) {
        result.emplace_back(cc.raw_channel);
    }
    return result;
}

core::frame_producer_dependencies get_producer_dependencies(const std::shared_ptr<core::video_channel>& channel,
                                                            const command_context&                      ctx)
{
    return core::frame_producer_dependencies(channel->frame_factory(),
                                             get_channels(ctx),
                                             ctx.static_context->format_repository,
                                             channel->stage()->video_format_desc(),
                                             ctx.static_context->producer_registry);
}

// Basic Commands

std::wstring log_level_command(command_context& ctx)
{
    if (ctx.parameters.size() == 0) {
        std::wstringstream replyString;
        replyString << L"201 LOG OK\r\n" << boost::to_upper_copy(log::get_log_level()) << L"\r\n";

        return replyString.str();
    }

    if (!log::set_log_level(ctx.parameters.at(0))) {
        return L"403 LOG FAILED\r\n";
    }

    return L"202 LOG OK\r\n";
}

// Template Graphics Commands

std::wstring cg_add_command(command_context& ctx)
{
    // CG 1 ADD 0 "template_folder/templatename" [STARTLABEL] 0/1 [DATA]

    int          layer = std::stoi(ctx.parameters.at(0));
    std::wstring label;             //_parameters[2]
    bool         bDoStart  = false; //_parameters[2] alt. _parameters[3]
    unsigned int dataIndex = 3;

    if (ctx.parameters.at(2).length() > 1) { // read label
        label = ctx.parameters.at(2);
        ++dataIndex;

        if (ctx.parameters.at(3).length() > 0) // read play-on-load-flag
            bDoStart = ctx.parameters.at(3).at(0) == L'1' ? true : false;
    } else { // read play-on-load-flag
        bDoStart = ctx.parameters.at(2).at(0) == L'1' ? true : false;
    }

    const wchar_t* pDataString = nullptr;
    std::wstring   dataFromFile;
    if (ctx.parameters.size() > dataIndex) { // read data
        const std::wstring& dataString = ctx.parameters.at(dataIndex);

        if (dataString.at(0) == L'<' || dataString.at(0) == L'{') // the data is XML or Json
            pDataString = dataString.c_str();
        else {
            // The data is not an XML-string, it must be a filename
            std::wstring filename = env::data_folder();
            filename.append(dataString);
            filename.append(L".ftd");

            auto found_file = find_case_insensitive(filename);

            if (found_file) {
                dataFromFile = read_file(boost::filesystem::path(*found_file));
                pDataString  = dataFromFile.c_str();
            }
        }
    }

    auto filename = ctx.parameters.at(1);
    auto proxy =
        ctx.static_context->cg_registry->get_or_create_proxy(spl::make_shared_ptr(ctx.channel.raw_channel),
                                                             get_producer_dependencies(ctx.channel.raw_channel, ctx),
                                                             ctx.layer_index(core::cg_proxy::DEFAULT_LAYER),
                                                             filename);

    if (proxy == core::cg_proxy::empty())
        CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"Could not find template " + filename));
    else
        proxy->add(layer, filename, bDoStart, label, pDataString != nullptr ? pDataString : L"");

    return L"202 CG OK\r\n";
}

std::wstring cg_play_command(command_context& ctx)
{
    int layer = std::stoi(ctx.parameters.at(0));
    ctx.static_context->cg_registry
        ->get_proxy(spl::make_shared_ptr(ctx.channel.raw_channel), ctx.layer_index(core::cg_proxy::DEFAULT_LAYER))
        ->play(layer);

    return L"202 CG OK\r\n";
}

spl::shared_ptr<core::cg_proxy> get_expected_cg_proxy(command_context& ctx)
{
    auto proxy = ctx.static_context->cg_registry->get_proxy(spl::make_shared_ptr(ctx.channel.raw_channel),
                                                            ctx.layer_index(core::cg_proxy::DEFAULT_LAYER));

    if (proxy == cg_proxy::empty())
        CASPAR_THROW_EXCEPTION(expected_user_error() << msg_info(L"No CG proxy running on layer"));

    return proxy;
}

std::wstring cg_stop_command(command_context& ctx)
{
    int layer = std::stoi(ctx.parameters.at(0));
    get_expected_cg_proxy(ctx)->stop(layer);

    return L"202 CG OK\r\n";
}

std::wstring cg_next_command(command_context& ctx)
{
    int layer = std::stoi(ctx.parameters.at(0));
    get_expected_cg_proxy(ctx)->next(layer);

    return L"202 CG OK\r\n";
}

std::wstring cg_remove_command(command_context& ctx)
{
    int layer = std::stoi(ctx.parameters.at(0));
    get_expected_cg_proxy(ctx)->remove(layer);

    return L"202 CG OK\r\n";
}

std::wstring cg_clear_command(command_context& ctx)
{
    ctx.channel.stage->clear(ctx.layer_index(core::cg_proxy::DEFAULT_LAYER));

    return L"202 CG OK\r\n";
}

std::wstring cg_update_command(command_context& ctx)
{
    int layer = std::stoi(ctx.parameters.at(0));

    std::wstring dataString = ctx.parameters.at(1);
    if (dataString.at(0) != L'<' && dataString.at(0) != L'{') {
        // The data is not XML or Json, it must be a filename
        std::wstring filename = env::data_folder();
        filename.append(dataString);
        filename.append(L".ftd");

        dataString = read_file(boost::filesystem::path(filename));
    }

    get_expected_cg_proxy(ctx)->update(layer, dataString);

    return L"202 CG OK\r\n";
}

std::wstring cg_invoke_command(command_context& ctx)
{
    std::wstringstream replyString;
    replyString << L"201 CG OK\r\n";
    int  layer  = std::stoi(ctx.parameters.at(0));
    auto result = get_expected_cg_proxy(ctx)->invoke(layer, ctx.parameters.at(1));
    replyString << result << L"\r\n";

    return replyString.str();
}

// Mixer Commands

std::future<core::frame_transform> get_current_transform(command_context& ctx)
{
    return ctx.channel.stage->get_current_transform(ctx.layer_index());
}

template <typename Func>
std::future<std::wstring> reply_value(command_context& ctx, const Func& extractor)
{
    auto transform = get_current_transform(ctx).share();

    return std::async(std::launch::deferred, [transform, extractor]() -> std::wstring {
        auto value = extractor(transform.get());
        return L"201 MIXER OK\r\n" + boost::lexical_cast<std::wstring>(value) + L"\r\n";
    });
}

class transforms_applier
{
    std::vector<stage::transform_tuple_t> transforms_;
    command_context&                      ctx_;
    bool                                  defer_;

  public:
    explicit transforms_applier(command_context& ctx)
        : ctx_(ctx)
    {
        defer_ = !ctx.parameters.empty() && boost::iequals(ctx.parameters.back(), L"DEFER");

        if (defer_)
            ctx.parameters.pop_back();
    }

    void add(stage::transform_tuple_t&& transform) { transforms_.push_back(std::move(transform)); }

    void apply() {}
};

std::wstring mixer_mastervolume_command(command_context& ctx)
{
    if (ctx.parameters.empty()) {
        auto volume = ctx.channel.raw_channel->mixer().get_master_volume();
        return L"201 MIXER OK\r\n" + std::to_wstring(volume) + L"\r\n";
    }

    float master_volume = boost::lexical_cast<float>(ctx.parameters.at(0));
    ctx.channel.raw_channel->mixer().set_master_volume(master_volume);

    return L"202 MIXER OK\r\n";
}

std::wstring mixer_grid_command(command_context& ctx)
{
    // Reimplemented already

    return L"202 MIXER OK\r\n";
}

std::wstring mixer_clear_command(command_context& ctx)
{
    int layer = ctx.layer_id;

    if (layer == -1)
        ctx.channel.stage->clear_transforms();
    else
        ctx.channel.stage->clear_transforms(layer);

    return L"202 MIXER OK\r\n";
}

std::wstring channel_grid_command(command_context& ctx)
{
    int   index = 1;
    auto& self  = ctx.channels->back();

    core::diagnostics::scoped_call_context save;
    core::diagnostics::call_context::for_thread().video_channel = ctx.channels->size();

    std::vector<std::wstring> params;
    params.emplace_back(L"SCREEN");
    params.emplace_back(L"0");
    params.emplace_back(L"NAME");
    params.emplace_back(L"Channel Grid Window");
    auto screen = ctx.static_context->consumer_registry->create_consumer(
        params, ctx.static_context->format_repository, get_channels(ctx));

    self.raw_channel->output().add(screen);

    for (auto& ch : *ctx.channels) {
        if (ch.raw_channel != self.raw_channel) {
            core::diagnostics::call_context::for_thread().layer = index;
            auto producer = ctx.static_context->producer_registry->create_producer(
                get_producer_dependencies(self.raw_channel, ctx),
                L"route://" + std::to_wstring(ch.raw_channel->index()));
            self.stage->load(index, producer, false);
            self.stage->play(index);
            index++;
        }
    }

    auto num_channels       = ctx.channels->size() - 1;
    int  square_side_length = std::ceil(std::sqrt(num_channels));

    auto ctx2 = command_context(
        ctx.static_context, ctx.channels, ctx.client_address, self, self.raw_channel->index(), ctx.layer_id);
    ctx2.parameters.push_back(std::to_wstring(square_side_length));
    mixer_grid_command(ctx2);

    return L"202 CHANNEL_GRID OK\r\n";
}

// Query Commands

std::wstring gl_info_command(command_context& ctx)
{
    auto device = ctx.static_context->ogl_device.lock();
    if (!device)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("GL command only supported with OpenGL accelerator."));

    std::wstringstream result;
    result << L"201 GL INFO OK\r\n";

    pt::xml_writer_settings<std::wstring> w(' ', 3);
    pt::xml_parser::write_xml(result, device->info(), w);
    result << L"\r\n";

    return result.str();
}

std::wstring gl_gc_command(command_context& ctx)
{
    auto device = ctx.static_context->ogl_device.lock();
    if (!device)
        CASPAR_THROW_EXCEPTION(not_supported() << msg_info("GL command only supported with OpenGL accelerator."));

    device->gc().wait();

    return L"202 GL GC OK\r\n";
}

void register_commands(std::shared_ptr<amcp_command_repository_wrapper>& repo)
{
    repo->register_command(L"Basic Commands", L"LOG LEVEL", log_level_command, 0);

    repo->register_channel_command(L"Template Commands", L"CG ADD", cg_add_command, 3);
    repo->register_channel_command(L"Template Commands", L"CG PLAY", cg_play_command, 1);
    repo->register_channel_command(L"Template Commands", L"CG STOP", cg_stop_command, 1);
    repo->register_channel_command(L"Template Commands", L"CG NEXT", cg_next_command, 1);
    repo->register_channel_command(L"Template Commands", L"CG REMOVE", cg_remove_command, 1);
    repo->register_channel_command(L"Template Commands", L"CG CLEAR", cg_clear_command, 0);
    repo->register_channel_command(L"Template Commands", L"CG UPDATE", cg_update_command, 2);
    repo->register_channel_command(L"Template Commands", L"CG INVOKE", cg_invoke_command, 2);

    repo->register_channel_command(L"Mixer Commands", L"MIXER MASTERVOLUME", mixer_mastervolume_command, 0);
    repo->register_channel_command(L"Mixer Commands", L"MIXER CLEAR", mixer_clear_command, 0);
    repo->register_command(L"Mixer Commands", L"CHANNEL_GRID", channel_grid_command, 0);

    repo->register_command(L"Query Commands", L"INFO CONFIG", info_config_command, 0);

    repo->register_command(L"Query Commands", L"GL INFO", gl_info_command, 0);
    repo->register_command(L"Query Commands", L"GL GC", gl_gc_command, 0);
}
}}} // namespace caspar::protocol::amcp
