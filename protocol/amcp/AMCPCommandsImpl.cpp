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
#pragma warning (push, 1) // TODO: Legacy code, just disable warnings
#endif

#include "AMCPCommandsImpl.h"

#include "amcp_command_repository.h"
#include "AMCPCommandQueue.h"

#include <common/env.h>

#include <common/log.h>
#include <common/param.h>
#include <common/os/system_info.h>
#include <common/os/filesystem.h>
#include <common/base64.h>
#include <common/thread_info.h>
#include <common/filesystem.h>

#include <core/producer/cg_proxy.h>
#include <core/producer/frame_producer.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>
#include <core/help/util.h>
#include <core/video_format.h>
#include <core/producer/transition/transition_producer.h>
#include <core/frame/audio_channel_layout.h>
#include <core/frame/frame_transform.h>
#include <core/producer/text/text_producer.h>
#include <core/producer/stage.h>
#include <core/producer/layer.h>
#include <core/mixer/mixer.h>
#include <core/consumer/output.h>
#include <core/thumbnail_generator.h>
#include <core/producer/media_info/media_info.h>
#include <core/producer/media_info/media_info_repository.h>
#include <core/diagnostics/call_context.h>
#include <core/diagnostics/osd_graph.h>
#include <core/system_info_provider.h>

#include <algorithm>
#include <locale>
#include <fstream>
#include <memory>
#include <cctype>
#include <future>

#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/regex.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/locale.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/archive/iterators/base64_from_binary.hpp>
#include <boost/archive/iterators/insert_linebreaks.hpp>
#include <boost/archive/iterators/transform_width.hpp>

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

500 FAILED				internal error
501 [command] FAILED	internal error
502 [command] FAILED	could not read file
503 [command] FAILED	access denied

600 [command] FAILED	[command] not implemented
*/

namespace caspar { namespace protocol { namespace amcp {

using namespace core;

std::wstring read_file_base64(const boost::filesystem::path& file)
{
	using namespace boost::archive::iterators;

	boost::filesystem::ifstream filestream(file, std::ios::binary);

	if (!filestream)
		return L"";

	auto length = boost::filesystem::file_size(file);
	std::vector<char> bytes;
	bytes.resize(length);
	filestream.read(bytes.data(), length);

	std::string result(to_base64(bytes.data(), length));
	return std::wstring(result.begin(), result.end());
}

std::wstring read_utf8_file(const boost::filesystem::path& file)
{
	std::wstringstream result;
	boost::filesystem::wifstream filestream(file);

	if (filestream) 
	{
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
	gen.categories(boost::locale::codepage_facet);

	std::stringstream result_stream;
	boost::filesystem::ifstream filestream(file);
	filestream.imbue(gen("en_US.ISO8859-1"));

	if (filestream)
	{
		// read all data
		result_stream << filestream.rdbuf();
	}

	std::string result = result_stream.str();
	std::wstring widened_result;

	// The first 255 codepoints in unicode is the same as in latin1
	boost::copy(
		result | boost::adaptors::transformed(
				[](char c) { return static_cast<unsigned char>(c); }),
		std::back_inserter(widened_result));

	return widened_result;
}

std::wstring read_file(const boost::filesystem::path& file)
{
	static const uint8_t BOM[] = {0xef, 0xbb, 0xbf};

	if (!boost::filesystem::exists(file))
	{
		return L"";
	}

	if (boost::filesystem::file_size(file) >= 3)
	{
		boost::filesystem::ifstream bom_stream(file);

		char header[3];
		bom_stream.read(header, 3);
		bom_stream.close();

		if (std::memcmp(BOM, header, 3) == 0)
			return read_utf8_file(file);
	}

	return read_latin1_file(file);
}

std::wstring MediaInfo(const boost::filesystem::path& path, const spl::shared_ptr<media_info_repository>& media_info_repo)
{
	if (!boost::filesystem::is_regular_file(path))
		return L"";

	auto media_info = media_info_repo->get(path.wstring());

	if (!media_info)
		return L"";

	auto is_not_digit = [](char c){ return std::isdigit(c) == 0; };

	auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(path)));
	writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), is_not_digit), writeTimeStr.end());
	auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

	auto sizeStr = boost::lexical_cast<std::wstring>(boost::filesystem::file_size(path));
	sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), is_not_digit), sizeStr.end());
	auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

	auto relativePath = get_relative_without_extension(path, env::media_folder());
	auto str = relativePath.generic_wstring();

	if (str[0] == '\\' || str[0] == '/')
		str = std::wstring(str.begin() + 1, str.end());

	return std::wstring()
		+ L"\"" + str +
		+ L"\" " + media_info->clip_type +
		+ L" " + sizeStr +
		+ L" " + writeTimeWStr +
		+ L" " + boost::lexical_cast<std::wstring>(media_info->duration) +
		+ L" " + boost::lexical_cast<std::wstring>(media_info->time_base.numerator()) + L"/" + boost::lexical_cast<std::wstring>(media_info->time_base.denominator())
		+ L"\r\n";
}

std::wstring ListMedia(const spl::shared_ptr<media_info_repository>& media_info_repo)
{	
	std::wstringstream replyString;
	for (boost::filesystem::recursive_directory_iterator itr(env::media_folder()), end; itr != end; ++itr)
		replyString << MediaInfo(itr->path(), media_info_repo);
	
	return boost::to_upper_copy(replyString.str());
}

std::wstring ListTemplates(const spl::shared_ptr<core::cg_producer_registry>& cg_registry)
{
	std::wstringstream replyString;

	for (boost::filesystem::recursive_directory_iterator itr(env::template_folder()), end; itr != end; ++itr)
	{		
		if(boost::filesystem::is_regular_file(itr->path()) && cg_registry->is_cg_extension(itr->path().extension().wstring()))
		{
			auto relativePath = get_relative_without_extension(itr->path(), env::template_folder());

			auto writeTimeStr = boost::posix_time::to_iso_string(boost::posix_time::from_time_t(boost::filesystem::last_write_time(itr->path())));
			writeTimeStr.erase(std::remove_if(writeTimeStr.begin(), writeTimeStr.end(), [](char c){ return std::isdigit(c) == 0;}), writeTimeStr.end());
			auto writeTimeWStr = std::wstring(writeTimeStr.begin(), writeTimeStr.end());

			auto sizeStr = boost::lexical_cast<std::string>(boost::filesystem::file_size(itr->path()));
			sizeStr.erase(std::remove_if(sizeStr.begin(), sizeStr.end(), [](char c){ return std::isdigit(c) == 0;}), sizeStr.end());

			auto sizeWStr = std::wstring(sizeStr.begin(), sizeStr.end());

			auto dir = relativePath.parent_path();
			auto file = boost::to_upper_copy(relativePath.filename().wstring());
			relativePath = dir / file;
						
			auto str = relativePath.generic_wstring();
			boost::trim_if(str, boost::is_any_of("\\/"));

			auto template_type = cg_registry->get_cg_producer_name(str);

			replyString << L"\"" << str
						<< L"\" " << sizeWStr
						<< L" " << writeTimeWStr
						<< L" " << template_type
						<< L"\r\n";
		}
	}
	return replyString.str();
}

core::frame_producer_dependencies get_producer_dependencies(const std::shared_ptr<core::video_channel>& channel, const command_context& ctx)
{
	return core::frame_producer_dependencies(
			channel->frame_factory(),
			cpplinq::from(ctx.channels)
					.select([](channel_context c) { return spl::make_shared_ptr(c.channel); })
					.to_vector(),
			channel->video_format_desc(),
			ctx.producer_registry);
}

// Basic Commands

void loadbg_describer(core::help_sink& sink, const core::help_repository& repository)
{
	sink.short_description(L"Load a media file or resource in the background.");
	sink.syntax(LR"(LOADBG [channel:int]{-[layer:int]} [clip:string] {[loop:LOOP]} {[transition:CUT,MIX,PUSH,WIPE,SLIDE] [duration:int] {[tween:string]|linear} {[direction:LEFT,RIGHT]|RIGHT}|CUT 0} {SEEK [frame:int]} {LENGTH [frames:int]} {FILTER [filter:string]} {[auto:AUTO]})");
	sink.para()
		->text(L"Loads a producer in the background and prepares it for playout. ")
		->text(L"If no layer is specified the default layer index will be used.");
	sink.para()
		->code(L"clip")->text(L" will be parsed by available registered producer factories. ")
		->text(L"If a successfully match is found, the producer will be loaded into the background.");
	sink.para()
		->text(L"If a file with the same name (extension excluded) but with the additional postfix ")
		->code(L"_a")->text(L" is found this file will be used as key for the main ")->code(L"clip")->text(L".");
	sink.para()
		->code(L"loop")->text(L" will cause the clip to loop.");
	sink.para()
		->text(L"When playing and looping the clip will start at ")->code(L"frame")->text(L".");
	sink.para()
		->text(L"When playing and loop the clip will end after ")->code(L"frames")->text(L" number of frames.");
	sink.para()
		->code(L"auto")->text(L" will cause the clip to automatically start when foreground clip has ended (without play). ")
		->text(LR"(The clip is considered "started" after the optional transition has ended.)");
	sink.para()->text(L"Examples:");
	sink.example(L">> LOADBG 1-1 MY_FILE PUSH 20 easeinesine LOOP SEEK 200 LENGTH 400 AUTO FILTER hflip");
	sink.example(L">> LOADBG 1 MY_FILE PUSH 20 EASEINSINE");
	sink.example(L">> LOADBG 1-1 MY_FILE SLIDE 10 LEFT");
	sink.example(L">> LOADBG 1-0 MY_FILE");
	sink.example(
			L">> PLAY 1-1 MY_FILE\n"
			L">> LOADBG 1-1 EMPTY MIX 20 AUTO",
			L"To automatically fade out a layer after a video file has been played to the end");
	sink.para()
		->text(L"See ")->see(L"Animation Types")->text(L" for supported values for ")->code(L"tween")->text(L".");
	sink.para()
		->text(L"See ")->url(L"http://libav.org/libavfilter.html")->text(L" for supported values for the ")
		->code(L"filter")->text(L" command.");
}

std::wstring loadbg_command(command_context& ctx)
{
	transition_info transitionInfo;

	// TRANSITION

	std::wstring message;
	for (size_t n = 0; n < ctx.parameters.size(); ++n)
		message += boost::to_upper_copy(ctx.parameters[n]) + L" ";

	static const boost::wregex expr(LR"(.*(?<TRANSITION>CUT|PUSH|SLIDE|WIPE|MIX)\s*(?<DURATION>\d+)\s*(?<TWEEN>(LINEAR)|(EASE[^\s]*))?\s*(?<DIRECTION>FROMLEFT|FROMRIGHT|LEFT|RIGHT)?.*)");
	boost::wsmatch what;
	if (boost::regex_match(message, what, expr))
	{
		auto transition = what["TRANSITION"].str();
		transitionInfo.duration = boost::lexical_cast<size_t>(what["DURATION"].str());
		auto direction = what["DIRECTION"].matched ? what["DIRECTION"].str() : L"";
		auto tween = what["TWEEN"].matched ? what["TWEEN"].str() : L"";
		transitionInfo.tweener = tween;

		if (transition == L"CUT")
			transitionInfo.type = transition_type::cut;
		else if (transition == L"MIX")
			transitionInfo.type = transition_type::mix;
		else if (transition == L"PUSH")
			transitionInfo.type = transition_type::push;
		else if (transition == L"SLIDE")
			transitionInfo.type = transition_type::slide;
		else if (transition == L"WIPE")
			transitionInfo.type = transition_type::wipe;

		if (direction == L"FROMLEFT")
			transitionInfo.direction = transition_direction::from_left;
		else if (direction == L"FROMRIGHT")
			transitionInfo.direction = transition_direction::from_right;
		else if (direction == L"LEFT")
			transitionInfo.direction = transition_direction::from_right;
		else if (direction == L"RIGHT")
			transitionInfo.direction = transition_direction::from_left;
	}

	//Perform loading of the clip
	core::diagnostics::scoped_call_context save;
	core::diagnostics::call_context::for_thread().video_channel = ctx.channel_index + 1;
	core::diagnostics::call_context::for_thread().layer = ctx.layer_index();

	auto channel = ctx.channel.channel;
	auto pFP = ctx.producer_registry->create_producer(get_producer_dependencies(channel, ctx), ctx.parameters);

	if (pFP == frame_producer::empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(ctx.parameters.size() > 0 ? ctx.parameters[0] : L""));

	bool auto_play = contains_param(L"AUTO", ctx.parameters);

	auto pFP2 = create_transition_producer(channel->video_format_desc().field_mode, pFP, transitionInfo);
	if (auto_play)
		channel->stage().load(ctx.layer_index(), pFP2, false, transitionInfo.duration); // TODO: LOOP
	else
		channel->stage().load(ctx.layer_index(), pFP2, false); // TODO: LOOP

	return L"202 LOADBG OK\r\n";
}

void load_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Load a media file or resource to the foreground.");
	sink.syntax(LR"(LOAD [video_channel:int]{-[layer:int]|-0} [clip:string] {"additional parameters"})");
	sink.para()
		->text(L"Loads a clip to the foreground and plays the first frame before pausing. ")
		->text(L"If any clip is playing on the target foreground then this clip will be replaced.");
	sink.para()->text(L"Examples:");
	sink.example(L">> LOAD 1 MY_FILE");
	sink.example(L">> LOAD 1-1 MY_FILE");
	sink.para()->text(L"Note: See ")->see(L"LOADBG")->text(L" for additional details.");
}

std::wstring load_command(command_context& ctx)
{
	core::diagnostics::scoped_call_context save;
	core::diagnostics::call_context::for_thread().video_channel = ctx.channel_index + 1;
	core::diagnostics::call_context::for_thread().layer = ctx.layer_index();
	auto pFP = ctx.producer_registry->create_producer(get_producer_dependencies(ctx.channel.channel, ctx), ctx.parameters);
	ctx.channel.channel->stage().load(ctx.layer_index(), pFP, true);

	return L"202 LOAD OK\r\n";
}

void play_describer(core::help_sink& sink, const core::help_repository& repository)
{
	sink.short_description(L"Play a media file or resource.");
	sink.syntax(LR"(PLAY [video_channel:int]{-[layer:int]|-0} {[clip:string]} {"additional parameters"})");
	sink.para()
		->text(L"Moves clip from background to foreground and starts playing it. If a transition (see ")->see(L"LOADBG")
		->text(L") is prepared, it will be executed.");
	sink.para()
		->text(L"If additional parameters (see ")->see(L"LOADBG")
		->text(L") are provided then the provided clip will first be loaded to the background.");
	sink.para()->text(L"Examples:");
	sink.example(L">> PLAY 1 MY_FILE PUSH 20 EASEINSINE");
	sink.example(L">> PLAY 1-1 MY_FILE SLIDE 10 LEFT");
	sink.example(L">> PLAY 1-0 MY_FILE");
	sink.para()->text(L"Note: See ")->see(L"LOADBG")->text(L" for additional details.");
}

std::wstring play_command(command_context& ctx)
{
	if (!ctx.parameters.empty())
		loadbg_command(ctx);

	ctx.channel.channel->stage().play(ctx.layer_index());

	return L"202 PLAY OK\r\n";
}

void pause_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Pause playback of a layer.");
	sink.syntax(L"PAUSE [video_channel:int]{-[layer:int]|-0}");
	sink.para()
		->text(L"Pauses playback of the foreground clip on the specified layer. The ")->see(L"RESUME")
		->text(L" command can be used to resume playback again.");
	sink.para()->text(L"Examples:");
	sink.example(L">> PAUSE 1");
	sink.example(L">> PAUSE 1-1");
}

std::wstring pause_command(command_context& ctx)
{
	ctx.channel.channel->stage().pause(ctx.layer_index());
	return L"202 PAUSE OK\r\n";
}

void resume_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Resume playback of a layer.");
	sink.syntax(L"RESUME [video_channel:int]{-[layer:int]|-0}");
	sink.para()
		->text(L"Resumes playback of a foreground clip previously paused with the ")
		->see(L"PAUSE")->text(L" command.");
	sink.para()->text(L"Examples:");
	sink.example(L">> RESUME 1");
	sink.example(L">> RESUME 1-1");
}

std::wstring resume_command(command_context& ctx)
{
	ctx.channel.channel->stage().resume(ctx.layer_index());
	return L"202 RESUME OK\r\n";
}

void stop_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Remove the foreground clip of a layer.");
	sink.syntax(L"STOP [video_channel:int]{-[layer:int]|-0}");
	sink.para()
		->text(L"Removes the foreground clip of the specified layer.");
	sink.para()->text(L"Examples:");
	sink.example(L">> STOP 1");
	sink.example(L">> STOP 1-1");
}

std::wstring stop_command(command_context& ctx)
{
	ctx.channel.channel->stage().stop(ctx.layer_index());
	return L"202 STOP OK\r\n";
}

void clear_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Remove all clips of a layer or an entire channel.");
	sink.syntax(L"CLEAR [video_channel:int]{-[layer:int]}");
	sink.para()
		->text(L"Removes all clips (both foreground and background) of the specified layer. ")
		->text(L"If no layer is specified then all layers in the specified ")
		->code(L"video_channel")->text(L" are cleared.");
	sink.para()->text(L"Examples:");
	sink.example(L">> CLEAR 1", L"clears everything from the entire channel 1.");
	sink.example(L">> CLEAR 1-3", L"clears only layer 3 of channel 1.");
}

std::wstring clear_command(command_context& ctx)
{
	int index = ctx.layer_index(std::numeric_limits<int>::min());
	if (index != std::numeric_limits<int>::min())
		ctx.channel.channel->stage().clear(index);
	else
		ctx.channel.channel->stage().clear();

	return L"202 CLEAR OK\r\n";
}

void call_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Call a method on a producer.");
	sink.syntax(L"CALL [video_channel:int]{-[layer:int]|-0} [param:string]");
	sink.para()
		->text(L"Calls method on the specified producer with the provided ")
		->code(L"param")->text(L" string.");
	sink.para()->text(L"Examples:");
	sink.example(L">> CALL 1 LOOP");
	sink.example(L">> CALL 1-2 SEEK 25");
}

std::wstring call_command(command_context& ctx)
{
	auto result = ctx.channel.channel->stage().call(ctx.layer_index(), ctx.parameters);

	// TODO: because of std::async deferred timed waiting does not work

	/*auto wait_res = result.wait_for(std::chrono::seconds(2));
	if (wait_res == std::future_status::timeout)
	CASPAR_THROW_EXCEPTION(timed_out());*/

	std::wstringstream replyString;
	if (result.get().empty())
		replyString << L"202 CALL OK\r\n";
	else
		replyString << L"201 CALL OK\r\n" << result.get() << L"\r\n";

	return replyString.str();
}

void swap_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Swap layers between channels.");
	sink.syntax(L"SWAP [channel1:int]{-[layer1:int]} [channel2:int]{-[layer2:int]} {[transforms:TRANSFORMS]}");
	sink.para()
		->text(L"Swaps layers between channels (both foreground and background will be swapped). ")
		->text(L"By specifying ")->code(L"TRANSFORMS")->text(L" the transformations of the layers are swapped as well.");
	sink.para()->text(L"If layers are not specified then all layers in respective video channel will be swapped.");
	sink.para()->text(L"Examples:");
	sink.example(L">> SWAP 1 2");
	sink.example(L">> SWAP 1-1 2-3");
	sink.example(L">> SWAP 1-1 1-2");
	sink.example(L">> SWAP 1-1 1-2 TRANSFORMS", L"for swapping mixer transformations as well");
}

std::wstring swap_command(command_context& ctx)
{
	bool swap_transforms = ctx.parameters.size() > 1 && boost::iequals(ctx.parameters.at(1), L"TRANSFORMS");

	if (ctx.layer_index(-1) != -1)
	{
		std::vector<std::string> strs;
		boost::split(strs, ctx.parameters[0], boost::is_any_of("-"));

		auto ch1 = ctx.channel.channel;
		auto ch2 = ctx.channels.at(boost::lexical_cast<int>(strs.at(0)) - 1);

		int l1 = ctx.layer_index();
		int l2 = boost::lexical_cast<int>(strs.at(1));

		ch1->stage().swap_layer(l1, l2, ch2.channel->stage(), swap_transforms);
	}
	else
	{
		auto ch1 = ctx.channel.channel;
		auto ch2 = ctx.channels.at(boost::lexical_cast<int>(ctx.parameters[0]) - 1);
		ch1->stage().swap_layers(ch2.channel->stage(), swap_transforms);
	}

	return L"202 SWAP OK\r\n";
}

void add_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Add a consumer to a video channel.");
	sink.syntax(L"ADD [video_channel:int] [consumer:string] [parameters:string]");
	sink.para()
		->text(L"Adds a consumer to the specified video channel. The string ")
		->code(L"consumer")->text(L" will be parsed by the available consumer factories. ")
		->text(L"If a successful match is found a consumer will be created and added to the ")
		->code(L"video_channel")->text(L". Different consumers require different parameters, ")
		->text(L"some examples are below. Consumers can alternatively be specified by adding them to ")
		->see(L"the CasparCG config file")->text(L".");
	sink.para()->text(L"Examples:");
	sink.example(L">> ADD 1 DECKLINK 1");
	sink.example(L">> ADD 1 BLUEFISH 2");
	sink.example(L">> ADD 1 SCREEN");
	sink.example(L">> ADD 1 AUDIO");
	sink.example(L">> ADD 1 IMAGE filename");
	sink.example(L">> ADD 1 FILE filename.mov");
	sink.example(L">> ADD 1 FILE filename.mov SEPARATE_KEY");
	sink.para()->text(L"The streaming consumer is an implementation of the ffmpeg_consumer and supports many of the same arguments:");
	sink.example(L">> ADD 1 STREAM udp://localhost:5004 -vcodec libx264 -tune zerolatency -preset ultrafast -crf 25 -format mpegts -vf scale=240:180");
}

std::wstring add_command(command_context& ctx)
{
	replace_placeholders(
			L"<CLIENT_IP_ADDRESS>",
			ctx.client->address(),
			ctx.parameters);

	core::diagnostics::scoped_call_context save;
	core::diagnostics::call_context::for_thread().video_channel = ctx.channel_index + 1;

	auto consumer = ctx.consumer_registry->create_consumer(ctx.parameters, &ctx.channel.channel->stage());
	ctx.channel.channel->output().add(ctx.layer_index(consumer->index()), consumer);

	return L"202 ADD OK\r\n";
}

void remove_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Remove a consumer from a video channel.");
	sink.syntax(L"REMOVE [video_channel:int]{-[consumer_index:int]} {[parameters:string]}");
	sink.para()
		->text(L"Removes an existing consumer from ")->code(L"video_channel")
		->text(L". If ")->code(L"consumer_index")->text(L" is given, the consumer will be removed via its id. If ")
		->code(L"parameters")->text(L" are given instead, the consumer matching those parameters will be removed.");
	sink.para()->text(L"Examples:");
	sink.example(L">> REMOVE 1 DECKLINK 1");
	sink.example(L">> REMOVE 1 BLUEFISH 2");
	sink.example(L">> REMOVE 1 SCREEN");
	sink.example(L">> REMOVE 1 AUDIO");
	sink.example(L">> REMOVE 1-300", L"for removing the consumer with index 300 from channel 1");
}

std::wstring remove_command(command_context& ctx)
{
	auto index = ctx.layer_index(std::numeric_limits<int>::min());
	
	if (index == std::numeric_limits<int>::min())
	{
		replace_placeholders(
				L"<CLIENT_IP_ADDRESS>",
				ctx.client->address(),
				ctx.parameters);

		index = ctx.consumer_registry->create_consumer(ctx.parameters, &ctx.channel.channel->stage())->index();
	}

	ctx.channel.channel->output().remove(index);

	return L"202 REMOVE OK\r\n";
}

void print_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Take a snapshot of a channel.");
	sink.syntax(L"PRINT [video_channel:int]");
	sink.para()
		->text(L"Saves an RGBA PNG bitmap still image of the contents of the specified channel in the ")
		->code(L"media")->text(L" folder.");
	sink.para()->text(L"Examples:");
	sink.example(L">> PRINT 1", L"will produce a PNG image with the current date and time as the filename for example 20130620T192220.png");
}

std::wstring print_command(command_context& ctx)
{
	ctx.channel.channel->output().add(ctx.consumer_registry->create_consumer({ L"IMAGE" }, &ctx.channel.channel->stage()));

	return L"202 PRINT OK\r\n";
}

void log_level_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the log level of the server.");
	sink.syntax(L"LOG LEVEL [level:trace,debug,info,warning,error,fatal]");
	sink.para()->text(L"Changes the log level of the server.");
	sink.para()->text(L"Examples:");
	sink.example(L">> LOG LEVEL trace");
	sink.example(L">> LOG LEVEL info");
}

std::wstring log_level_command(command_context& ctx)
{
	log::set_log_level(ctx.parameters.at(0));

	return L"202 LOG OK\r\n";
}

void set_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the value of a channel variable.");
	sink.syntax(L"SET [video_channel:int] [variable:string] [value:string]");
	sink.para()->text(L"Changes the value of a channel variable. Available variables to set:");
	sink.definitions()
		->item(L"MODE", L"Changes the video format of the channel.")
		->item(L"CHANNEL_LAYOUT", L"Changes the audio channel layout of the video channel channel.");
	sink.para()->text(L"Examples:");
	sink.example(L">> SET 1 MODE PAL", L"changes the video mode on channel 1 to PAL.");
	sink.example(L">> SET 1 CHANNEL_LAYOUT smpte", L"changes the audio channel layout on channel 1 to smpte.");
}

std::wstring set_command(command_context& ctx)
{
	std::wstring name = boost::to_upper_copy(ctx.parameters[0]);
	std::wstring value = boost::to_upper_copy(ctx.parameters[1]);

	if (name == L"MODE")
	{
		auto format_desc = core::video_format_desc(value);
		if (format_desc.format != core::video_format::invalid)
		{
			ctx.channel.channel->video_format_desc(format_desc);
			return L"202 SET MODE OK\r\n";
		}

		CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid video mode"));
	}
	else if (name == L"CHANNEL_LAYOUT")
	{
		auto channel_layout = core::audio_channel_layout_repository::get_default()->get_layout(value);

		if (channel_layout)
		{
			ctx.channel.channel->audio_channel_layout(*channel_layout);
			return L"202 SET CHANNEL_LAYOUT OK\r\n";
		}

		CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid audio channel layout"));
	}

	CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Invalid channel variable"));
}

void data_store_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Store a dataset.");
	sink.syntax(L"DATA STORE [name:string] [data:string]");
	sink.para()->text(L"Stores the dataset data under the name ")->code(L"name")->text(L".");
	sink.para()->text(L"Directories will be created if they do not exist.");
	sink.para()->text(L"Examples:");
	sink.example(LR"(>> DATA STORE my_data "Some useful data")");
	sink.example(LR"(>> DATA STORE Folder1/my_data "Some useful data")");
}

std::wstring data_store_command(command_context& ctx)
{
	std::wstring filename = env::data_folder();
	filename.append(ctx.parameters[0]);
	filename.append(L".ftd");

	auto data_path = boost::filesystem::path(filename).parent_path().wstring();
	auto found_data_path = find_case_insensitive(data_path);

	if (found_data_path)
		data_path = *found_data_path;

	if (!boost::filesystem::exists(data_path))
		boost::filesystem::create_directories(data_path);

	auto found_filename = find_case_insensitive(filename);

	if (found_filename)
		filename = *found_filename; // Overwrite case insensitive.

	boost::filesystem::wofstream datafile(filename);
	if (!datafile)
		CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(L"Could not open file " + filename));

	datafile << static_cast<wchar_t>(65279); // UTF-8 BOM character
	datafile << ctx.parameters[1] << std::flush;
	datafile.close();

	return L"202 DATA STORE OK\r\n";
}

void data_retrieve_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Retrieve a dataset.");
	sink.syntax(L"DATA RETRIEVE [name:string] [data:string]");
	sink.para()->text(L"Returns the data saved under the name ")->code(L"name")->text(L".");
	sink.para()->text(L"Examples:");
	sink.example(L">> DATA RETRIEVE my_data");
	sink.example(L">> DATA RETRIEVE Folder1/my_data");
}

std::wstring data_retrieve_command(command_context& ctx)
{
	std::wstring filename = env::data_folder();
	filename.append(ctx.parameters[0]);
	filename.append(L".ftd");

	std::wstring file_contents;

	auto found_file = find_case_insensitive(filename);

	if (found_file)
		file_contents = read_file(boost::filesystem::path(*found_file));

	if (file_contents.empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(filename + L" not found"));

	std::wstringstream reply;
	reply << L"201 DATA RETRIEVE OK\r\n";

	std::wstringstream file_contents_stream(file_contents);
	std::wstring line;

	bool firstLine = true;
	while (std::getline(file_contents_stream, line))
	{
		if (firstLine)
			firstLine = false;
		else
			reply << "\n";

		reply << line;
	}

	reply << "\r\n";
	return reply.str();
}

void data_list_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"List stored datasets.");
	sink.syntax(L"DATA LIST");
	sink.para()->text(L"Returns a list of all stored datasets.");
}

std::wstring data_list_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"200 DATA LIST OK\r\n";

	for (boost::filesystem::recursive_directory_iterator itr(env::data_folder()), end; itr != end; ++itr)
	{
		if (boost::filesystem::is_regular_file(itr->path()))
		{
			if (!boost::iequals(itr->path().extension().wstring(), L".ftd"))
				continue;

			auto relativePath = get_relative_without_extension(itr->path(), env::data_folder());
			auto str = relativePath.generic_wstring();

			if (str[0] == L'\\' || str[0] == L'/')
				str = std::wstring(str.begin() + 1, str.end());

			replyString << str << L"\r\n";
		}
	}

	replyString << L"\r\n";

	return boost::to_upper_copy(replyString.str());
}

void data_remove_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Remove a stored dataset.");
	sink.syntax(L"DATA REMOVE [name:string]");
	sink.para()->text(L"Removes the dataset saved under the name ")->code(L"name")->text(L".");
	sink.para()->text(L"Examples:");
	sink.example(L">> DATA REMOVE my_data");
	sink.example(L">> DATA REMOVE Folder1/my_data");
}

std::wstring data_remove_command(command_context& ctx)
{
	std::wstring filename = env::data_folder();
	filename.append(ctx.parameters[0]);
	filename.append(L".ftd");

	if (!boost::filesystem::exists(filename))
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(filename + L" not found"));

	if (!boost::filesystem::remove(filename))
		CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(filename + L" could not be removed"));

	return L"202 DATA REMOVE OK\r\n";
}

// Template Graphics Commands

void cg_add_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Prepare a template for displaying.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} ADD [cg_layer:int] [template:string] [play-on-load:0,1] {[data]}");
	sink.para()
		->text(L"Prepares a template for displaying. It won't show until you call ")->see(L"CG PLAY")
		->text(L" (unless you supply the play-on-load flag, 1 for true). Data is either inline XML or a reference to a saved dataset.");
	sink.para()->text(L"Examples:");
	sink.example(L"CG 1 ADD 10 svtnews/info 1");
}

std::wstring cg_add_command(command_context& ctx)
{
	//CG 1 ADD 0 "template_folder/templatename" [STARTLABEL] 0/1 [DATA]

	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
	std::wstring label;		//_parameters[2]
	bool bDoStart = false;		//_parameters[2] alt. _parameters[3]
	unsigned int dataIndex = 3;

	if (ctx.parameters.at(2).length() > 1)
	{	//read label
		label = ctx.parameters.at(2);
		++dataIndex;

		if (ctx.parameters.at(3).length() > 0)	//read play-on-load-flag
			bDoStart = (ctx.parameters.at(3).at(0) == L'1') ? true : false;
	}
	else
	{	//read play-on-load-flag
		bDoStart = (ctx.parameters.at(2).at(0) == L'1') ? true : false;
	}

	const wchar_t* pDataString = 0;
	std::wstring dataFromFile;
	if (ctx.parameters.size() > dataIndex)
	{	//read data
		const std::wstring& dataString = ctx.parameters.at(dataIndex);

		if (dataString.at(0) == L'<' || dataString.at(0) == L'{') //the data is XML or Json
			pDataString = dataString.c_str();
		else
		{
			//The data is not an XML-string, it must be a filename
			std::wstring filename = env::data_folder();
			filename.append(dataString);
			filename.append(L".ftd");

			auto found_file = find_case_insensitive(filename);

			if (found_file)
			{
				dataFromFile = read_file(boost::filesystem::path(*found_file));
				pDataString = dataFromFile.c_str();
			}
		}
	}

	auto filename = ctx.parameters.at(1);
	auto proxy = ctx.cg_registry->get_or_create_proxy(
		spl::make_shared_ptr(ctx.channel.channel),
		get_producer_dependencies(ctx.channel.channel, ctx),
		ctx.layer_index(core::cg_proxy::DEFAULT_LAYER),
		filename);

	if (proxy == core::cg_proxy::empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"Could not find template " + filename));
	else
		proxy->add(layer, filename, bDoStart, label, (pDataString != 0) ? pDataString : L"");

	return L"202 CG OK\r\n";
}

void cg_play_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Play and display a template.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} PLAY [cg_layer:int]");
	sink.para()->text(L"Plays and displays the template in the specified layer.");
	sink.para()->text(L"Examples:");
	sink.example(L"CG 1 PLAY 0");
}

std::wstring cg_play_command(command_context& ctx)
{
	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
	ctx.cg_registry->get_proxy(spl::make_shared_ptr(ctx.channel.channel), ctx.layer_index(core::cg_proxy::DEFAULT_LAYER))->play(layer);

	return L"202 CG OK\r\n";
}

spl::shared_ptr<core::cg_proxy> get_expected_cg_proxy(command_context& ctx)
{
	auto proxy = ctx.cg_registry->get_proxy(spl::make_shared_ptr(ctx.channel.channel), ctx.layer_index(core::cg_proxy::DEFAULT_LAYER));

	if (proxy == cg_proxy::empty())
		CASPAR_THROW_EXCEPTION(expected_user_error() << msg_info(L"No CG proxy running on layer"));

	return proxy;
}

void cg_stop_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Stop and remove a template.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} STOP [cg_layer:int]");
	sink.para()
		->text(L"Stops and removes the template from the specified layer. This is different from ")->code(L"REMOVE")
		->text(L" in that the template gets a chance to animate out when it is stopped.");
	sink.para()->text(L"Examples:");
	sink.example(L"CG 1 STOP 0");
}

std::wstring cg_stop_command(command_context& ctx)
{
	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
	get_expected_cg_proxy(ctx)->stop(layer, 0);

	return L"202 CG OK\r\n";
}

void cg_next_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(LR"(Trigger a "continue" in a template.)");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} NEXT [cg_layer:int]");
	sink.para()
		->text(LR"(Triggers a "continue" in the template on the specified layer. )")
		->text(L"This is used to control animations that has multiple discreet steps.");
	sink.para()->text(L"Examples:");
	sink.example(L"CG 1 NEXT 0");
}

std::wstring cg_next_command(command_context& ctx)
{
	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
	get_expected_cg_proxy(ctx)->next(layer);

	return L"202 CG OK\r\n";
}

void cg_remove_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Remove a template.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} REMOVE [cg_layer:int]");
	sink.para()->text(L"Removes the template from the specified layer.");
	sink.para()->text(L"Examples:");
	sink.example(L"CG 1 REMOVE 0");
}

std::wstring cg_remove_command(command_context& ctx)
{
	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
	get_expected_cg_proxy(ctx)->remove(layer);

	return L"202 CG OK\r\n";
}

void cg_clear_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Remove all templates on a video layer.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} CLEAR");
	sink.para()->text(L"Removes all templates on a video layer. The entire cg producer will be removed.");
	sink.para()->text(L"Examples:");
	sink.example(L"CG 1 CLEAR");
}

std::wstring cg_clear_command(command_context& ctx)
{
	ctx.channel.channel->stage().clear(ctx.layer_index(core::cg_proxy::DEFAULT_LAYER));

	return L"202 CG OK\r\n";
}

void cg_update_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Update a template with new data.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} UPDATE [cg_layer:int] [data:string]");
	sink.para()->text(L"Sends new data to the template on specified layer. Data is either inline XML or a reference to a saved dataset.");
}

std::wstring cg_update_command(command_context& ctx)
{
	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));

	std::wstring dataString = ctx.parameters.at(1);
	if (dataString.at(0) != L'<' && dataString.at(0) != L'{')
	{
		//The data is not XML or Json, it must be a filename
		std::wstring filename = env::data_folder();
		filename.append(dataString);
		filename.append(L".ftd");

		dataString = read_file(boost::filesystem::path(filename));
	}

	get_expected_cg_proxy(ctx)->update(layer, dataString);

	return L"202 CG OK\r\n";
}

void cg_invoke_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Invoke a method/label on a template.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} INVOKE [cg_layer:int] [method:string]");
	sink.para()->text(L"Invokes the given method on the template on the specified layer.");
	sink.para()->text(L"Can be used to jump the playhead to a specific label.");
}

std::wstring cg_invoke_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"201 CG OK\r\n";
	int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
	auto result = get_expected_cg_proxy(ctx)->invoke(layer, ctx.parameters.at(1));
	replyString << result << L"\r\n";

	return replyString.str();
}

void cg_info_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get information about a running template or the template host.");
	sink.syntax(L"CG [video_channel:int]{-[layer:int]|-9999} INFO {[cg_layer:int]}");
	sink.para()->text(L"Retrieves information about the template on the specified layer.");
	sink.para()->text(L"If ")->code(L"cg_layer")->text(L" is not given, information about the template host is given instead.");
}

std::wstring cg_info_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"201 CG OK\r\n";

	if (ctx.parameters.empty())
	{
		auto info = get_expected_cg_proxy(ctx)->template_host_info();
		replyString << info << L"\r\n";
	}
	else
	{
		int layer = boost::lexical_cast<int>(ctx.parameters.at(0));
		auto desc = get_expected_cg_proxy(ctx)->description(layer);

		replyString << desc << L"\r\n";
	}

	return replyString.str();
}

// Mixer Commands

core::frame_transform get_current_transform(command_context& ctx)
{
	return ctx.channel.channel->stage().get_current_transform(ctx.layer_index()).get();
}

template<typename Func>
std::wstring reply_value(command_context& ctx, const Func& extractor)
{
	auto value = extractor(get_current_transform(ctx));

	return L"201 MIXER OK\r\n" + boost::lexical_cast<std::wstring>(value)+L"\r\n";
}

class transforms_applier
{
	static tbb::concurrent_unordered_map<int, std::vector<stage::transform_tuple_t>> deferred_transforms_;

	std::vector<stage::transform_tuple_t>	transforms_;
	command_context&						ctx_;
	bool									defer_;
public:
	transforms_applier(command_context& ctx)
		: ctx_(ctx)
	{
		defer_ = !ctx.parameters.empty() && boost::iequals(ctx.parameters.back(), L"DEFER");

		if (defer_)
			ctx.parameters.pop_back();
	}

	void add(stage::transform_tuple_t&& transform)
	{
		transforms_.push_back(std::move(transform));
	}

	void commit_deferred()
	{
		auto& transforms = deferred_transforms_[ctx_.channel_index];
		ctx_.channel.channel->stage().apply_transforms(transforms).get();
		transforms.clear();
	}

	void apply()
	{
		if (defer_)
		{
			auto& defer_tranforms = deferred_transforms_[ctx_.channel_index];
			defer_tranforms.insert(defer_tranforms.end(), transforms_.begin(), transforms_.end());
		}
		else
			ctx_.channel.channel->stage().apply_transforms(transforms_);
	}
};
tbb::concurrent_unordered_map<int, std::vector<stage::transform_tuple_t>> transforms_applier::deferred_transforms_;

void mixer_keyer_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Let a layer act as alpha for the one obove.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} KEYER {keyer:0,1|0}");
	sink.para()
		->text(L"Replaces layer ")->code(L"n+1")->text(L"'s alpha with the ")
		->code(L"R")->text(L" (red) channel of layer ")->code(L"n")
		->text(L", and hides the RGB channels of layer ")->code(L"n")
		->text(L". If keyer equals 1 then the specified layer will not be rendered, ")
		->text(L"instead it will be used as the key for the layer above.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 KEYER 1");
	sink.example(
		L">> MIXER 1-0 KEYER\n"
		L"<< 201 MIXER OK\n"
		L"<< 1", L"to retrieve the current state");
}

std::wstring mixer_keyer_command(command_context& ctx)
{
	if (ctx.parameters.empty())
		return reply_value(ctx, [](const frame_transform& t) { return t.image_transform.is_key ? 1 : 0; });

	transforms_applier transforms(ctx);
	bool value = boost::lexical_cast<int>(ctx.parameters.at(0));
	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.is_key = value;
		return transform;
	}, 0, L"linear"));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

std::wstring ANIMATION_SYNTAX = L" {[duration:int] {[tween:string]|linear}|0 linear}}";

void mixer_chroma_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Enable chroma keying on a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} CHROMA {[color:none,green,blue] {[threshold:float] [softness:float] {[spill:float]}}" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Enables or disables chroma keying on the specified video layer. Giving no parameters returns the current chroma settings.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-1 CHROMA green 0.10 0.20 25 easeinsine");
	sink.example(L">> MIXER 1-1 CHROMA none");
	sink.example(
		L">> MIXER 1-1 BLEND\n"
		L"<< 201 MIXER OK\n"
		L"<< SCREEN", L"for getting the current blend mode");
}

std::wstring mixer_chroma_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto chroma = get_current_transform(ctx).image_transform.chroma;
		return L"201 MIXER OK\r\n"
			+ core::get_chroma_mode(chroma.key) + L" "
			+ boost::lexical_cast<std::wstring>(chroma.threshold) + L" "
			+ boost::lexical_cast<std::wstring>(chroma.softness) + L" "
			+ boost::lexical_cast<std::wstring>(chroma.spill) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 4 ? boost::lexical_cast<int>(ctx.parameters.at(4)) : 0;
	std::wstring tween = ctx.parameters.size() > 5 ? ctx.parameters.at(5) : L"linear";

	core::chroma chroma;
	chroma.key = get_chroma_mode(ctx.parameters.at(0));

	if (chroma.key != core::chroma::type::none)
	{
		chroma.threshold = boost::lexical_cast<double>(ctx.parameters.at(1));
		chroma.softness = boost::lexical_cast<double>(ctx.parameters.at(2));
		chroma.spill = ctx.parameters.size() > 3 ? boost::lexical_cast<double>(ctx.parameters.at(3)) : 0.0;
	}

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.chroma = chroma;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_blend_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Set the blend mode for a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} BLEND {[blend:string]|normal}");
	sink.para()
		->text(L"Sets the blend mode to use when compositing this layer with the background. ")
		->text(L"If no argument is given the current blend mode is returned.");
	sink.para()
		->text(L"Every layer can be set to use a different ")->url(L"http://en.wikipedia.org/wiki/Blend_modes", L"blend mode")
		->text(L" than the default ")->code(L"normal")->text(L" mode, similar to applications like Photoshop. ")
		->text(L"Some common uses are to use ")->code(L"screen")->text(L" to make all the black image data become transparent, ")
		->text(L"or to use ")->code(L"add")->text(L" to selectively lighten highlights.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-1 BLEND OVERLAY");
	sink.example(
		L">> MIXER 1-1 BLEND\n"
		L"<< 201 MIXER OK\n"
		L"<< SCREEN", L"for getting the current blend mode");
	sink.para()->text(L"See ")->see(L"Blend Modes")->text(L" for supported values for ")->code(L"blend")->text(L".");
}

std::wstring mixer_blend_command(command_context& ctx)
{
	if (ctx.parameters.empty())
		return reply_value(ctx, [](const frame_transform& t) { return get_blend_mode(t.image_transform.blend_mode); });

	transforms_applier transforms(ctx);
	auto value = get_blend_mode(ctx.parameters.at(0));
	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.blend_mode = value;
		return transform;
	}, 0, L"linear"));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

template<typename Getter, typename Setter>
std::wstring single_double_animatable_mixer_command(command_context& ctx, const Getter& getter, const Setter& setter)
{
	if (ctx.parameters.empty())
		return reply_value(ctx, getter);

	transforms_applier transforms(ctx);
	double value = boost::lexical_cast<double>(ctx.parameters.at(0));
	int duration = ctx.parameters.size() > 1 ? boost::lexical_cast<int>(ctx.parameters[1]) : 0;
	std::wstring tween = ctx.parameters.size() > 2 ? ctx.parameters[2] : L"linear";

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		setter(transform, value);
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_opacity_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the opacity of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} OPACITY {[opacity:float]" + ANIMATION_SYNTAX);
	sink.para()->text(L"Changes the opacity of the specified layer. The value is a float between 0 and 1.");
	sink.para()->text(L"Retrieves the opacity of the specified layer if no argument is given.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 OPACITY 0.5 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 OPACITY\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.5", L"to retrieve the current opacity");
}

std::wstring mixer_opacity_command(command_context& ctx)
{
	return single_double_animatable_mixer_command(
			ctx,
			[](const frame_transform& t) { return t.image_transform.opacity; },
			[](frame_transform& t, double value) { t.image_transform.opacity = value; });
}

void mixer_brightness_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the brightness of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} BRIGHTNESS {[brightness:float]" + ANIMATION_SYNTAX);
	sink.para()->text(L"Changes the brightness of the specified layer. The value is a float between 0 and 1.");
	sink.para()->text(L"Retrieves the brightness of the specified layer if no argument is given.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 BRIGHTNESS 0.5 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 BRIGHTNESS\n"
		L"<< 201 MIXER OK\n"
		L"0.5", L"to retrieve the current brightness");
}

std::wstring mixer_brightness_command(command_context& ctx)
{
	return single_double_animatable_mixer_command(
			ctx,
			[](const frame_transform& t) { return t.image_transform.brightness; },
			[](frame_transform& t, double value) { t.image_transform.brightness = value; });
}

void mixer_saturation_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the saturation of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} SATURATION {[saturation:float]" + ANIMATION_SYNTAX);
	sink.para()->text(L"Changes the saturation of the specified layer. The value is a float between 0 and 1.");
	sink.para()->text(L"Retrieves the saturation of the specified layer if no argument is given.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 SATURATION 0.5 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 SATURATION\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.5", L"to retrieve the current saturation");
}

std::wstring mixer_saturation_command(command_context& ctx)
{
	return single_double_animatable_mixer_command(
			ctx,
			[](const frame_transform& t) { return t.image_transform.saturation; },
			[](frame_transform& t, double value) { t.image_transform.saturation = value; });
}

void mixer_contrast_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the contrast of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} CONTRAST {[contrast:float]" + ANIMATION_SYNTAX);
	sink.para()->text(L"Changes the contrast of the specified layer. The value is a float between 0 and 1.");
	sink.para()->text(L"Retrieves the contrast of the specified layer if no argument is given.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 CONTRAST 0.5 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 CONTRAST\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.5", L"to retrieve the current contrast");
}

std::wstring mixer_contrast_command(command_context& ctx)
{
	return single_double_animatable_mixer_command(
			ctx,
			[](const frame_transform& t) { return t.image_transform.contrast; },
			[](frame_transform& t, double value) { t.image_transform.contrast = value; });
}

void mixer_levels_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Adjust the video levels of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} LEVELS {[min-input:float] [max-input:float] [gamma:float] [min-output:float] [max-output:float]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Adjusts the video levels of a layer. If no arguments are given the current levels are returned.");
	sink.definitions()
		->item(L"min-input, max-input", L"Defines the input range (between 0 and 1) to accept RGB values within.")
		->item(L"gamma", L"Adjusts the gamma of the image.")
		->item(L"min-output, max-output", L"Defines the output range (between 0 and 1) to scale the accepted input RGB values to.");
		sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-10 LEVELS 0.0627 0.922 1 0 1 25 easeinsine", L"for stretching 16-235 video to 0-255 video");
	sink.example(L">> MIXER 1-10 LEVELS 0 1 1 0.0627 0.922 25 easeinsine", L"for compressing 0-255 video to 16-235 video");
	sink.example(L">> MIXER 1-10 LEVELS 0 1 0.5 0 1 25 easeinsine", L"for adjusting the gamma to 0.5");
	sink.example(
		L">> MIXER 1-10 LEVELS\n"
		L"<< 201 MIXER OK\n"
		L"<< 0 1 0.5 0 1", L"for retrieving the current levels");
}

std::wstring mixer_levels_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto levels = get_current_transform(ctx).image_transform.levels;
		return L"201 MIXER OK\r\n"
			+ boost::lexical_cast<std::wstring>(levels.min_input) + L" "
			+ boost::lexical_cast<std::wstring>(levels.max_input) + L" "
			+ boost::lexical_cast<std::wstring>(levels.gamma) + L" "
			+ boost::lexical_cast<std::wstring>(levels.min_output) + L" "
			+ boost::lexical_cast<std::wstring>(levels.max_output) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	levels value;
	value.min_input = boost::lexical_cast<double>(ctx.parameters.at(0));
	value.max_input = boost::lexical_cast<double>(ctx.parameters.at(1));
	value.gamma = boost::lexical_cast<double>(ctx.parameters.at(2));
	value.min_output = boost::lexical_cast<double>(ctx.parameters.at(3));
	value.max_output = boost::lexical_cast<double>(ctx.parameters.at(4));
	int duration = ctx.parameters.size() > 5 ? boost::lexical_cast<int>(ctx.parameters[5]) : 0;
	std::wstring tween = ctx.parameters.size() > 6 ? ctx.parameters[6] : L"linear";

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.levels = value;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_fill_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the fill position and scale of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} FILL {[x:float] [y:float] [x-scale:float] [y-scale:float]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Scales/positions the video stream on the specified layer. The concept is quite simple; ")
		->text(L"it comes from the ancient DVE machines like ADO. Imagine that the screen has a size of 1x1 ")
		->text(L"(not in pixel, but in an abstract measure). Then the coordinates of a full size picture is 0 0 1 1, ")
		->text(L"which means left edge is at coordinate 0, top edge at coordinate 0, width full size = 1, heigh full size = 1.");
	sink.para()
		->text(L"If you want to crop the picture on the left side (for wipe left to right) ")
		->text(L"You set the left edge to full right => 1 and the width to 0. So this give you the start-coordinates of 1 0 0 1.");
	sink.para()->text(L"End coordinates of any wipe are allways the full picture 0 0 1 1.");
	sink.para()
		->text(L"With the FILL command it can make sense to have values between 1 and 0, ")
		->text(L"if you want to do a smaller window. If, for instance you want to have a window of half the size of your screen, ")
		->text(L"you set with and height to 0.5. If you want to center it you set left and top edge to 0.25 so you will get the arguments 0.25 0.25 0.5 0.5");
	sink.definitions()
		->item(L"x", L"The new x position, 0 = left edge of monitor, 0.5 = middle of monitor, 1.0 = right edge of monitor. Higher and lower values allowed.")
		->item(L"y", L"The new y position, 0 = top edge of monitor, 0.5 = middle of monitor, 1.0 = bottom edge of monitor. Higher and lower values allowed.")
		->item(L"x-scale", L"The new x scale, 1 = 1x the screen width, 0.5 = half the screen width. Higher and lower values allowed. Negative values flips the layer.")
		->item(L"y-scale", L"The new y scale, 1 = 1x the screen height, 0.5 = half the screen height. Higher and lower values allowed. Negative values flips the layer.");
	sink.para()->text(L"The positioning and scaling is done around the anchor point set by ")->see(L"MIXER ANCHOR")->text(L".");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 FILL 0.25 0.25 0.5 0.5 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 FILL\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.25 0.25 0.5 0.5", L"gets the current fill");
}

std::wstring mixer_fill_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto transform = get_current_transform(ctx).image_transform;
		auto translation = transform.fill_translation;
		auto scale = transform.fill_scale;
		return L"201 MIXER OK\r\n"
			+ boost::lexical_cast<std::wstring>(translation[0]) + L" "
			+ boost::lexical_cast<std::wstring>(translation[1]) + L" "
			+ boost::lexical_cast<std::wstring>(scale[0]) + L" "
			+ boost::lexical_cast<std::wstring>(scale[1]) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 4 ? boost::lexical_cast<int>(ctx.parameters[4]) : 0;
	std::wstring tween = ctx.parameters.size() > 5 ? ctx.parameters[5] : L"linear";
	double x = boost::lexical_cast<double>(ctx.parameters.at(0));
	double y = boost::lexical_cast<double>(ctx.parameters.at(1));
	double x_s = boost::lexical_cast<double>(ctx.parameters.at(2));
	double y_s = boost::lexical_cast<double>(ctx.parameters.at(3));

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) mutable -> frame_transform
	{
		transform.image_transform.fill_translation[0] = x;
		transform.image_transform.fill_translation[1] = y;
		transform.image_transform.fill_scale[0] = x_s;
		transform.image_transform.fill_scale[1] = y_s;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_clip_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the clipping viewport of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} CLIP {[x:float] [y:float] [width:float] [height:float]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Defines the rectangular viewport where a layer is rendered thru on the screen without being affected by ")
		->see(L"MIXER FILL")->text(L", ")->see(L"MIXER ROTATION")->text(L" and ")->see(L"MIXER PERSPECTIVE")
		->text(L". See ")->see(L"MIXER CROP")->text(L" if you want to crop the layer before transforming it.");
	sink.definitions()
		->item(L"x", L"The new x position, 0 = left edge of monitor, 0.5 = middle of monitor, 1.0 = right edge of monitor. Higher and lower values allowed.")
		->item(L"y", L"The new y position, 0 = top edge of monitor, 0.5 = middle of monitor, 1.0 = bottom edge of monitor. Higher and lower values allowed.")
		->item(L"width", L"The new width, 1 = 1x the screen width, 0.5 = half the screen width. Higher and lower values allowed. Negative values flips the layer.")
		->item(L"height", L"The new height, 1 = 1x the screen height, 0.5 = half the screen height. Higher and lower values allowed. Negative values flips the layer.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 CLIP 0.25 0.25 0.5 0.5 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 CLIP\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.25 0.25 0.5 0.5", L"for retrieving the current clipping rect");
}

std::wstring mixer_clip_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto transform = get_current_transform(ctx).image_transform;
		auto translation = transform.clip_translation;
		auto scale = transform.clip_scale;

		return L"201 MIXER OK\r\n"
			+ boost::lexical_cast<std::wstring>(translation[0]) + L" "
			+ boost::lexical_cast<std::wstring>(translation[1]) + L" "
			+ boost::lexical_cast<std::wstring>(scale[0]) + L" "
			+ boost::lexical_cast<std::wstring>(scale[1]) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 4 ? boost::lexical_cast<int>(ctx.parameters[4]) : 0;
	std::wstring tween = ctx.parameters.size() > 5 ? ctx.parameters[5] : L"linear";
	double x = boost::lexical_cast<double>(ctx.parameters.at(0));
	double y = boost::lexical_cast<double>(ctx.parameters.at(1));
	double x_s = boost::lexical_cast<double>(ctx.parameters.at(2));
	double y_s = boost::lexical_cast<double>(ctx.parameters.at(3));

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.clip_translation[0] = x;
		transform.image_transform.clip_translation[1] = y;
		transform.image_transform.clip_scale[0] = x_s;
		transform.image_transform.clip_scale[1] = y_s;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_anchor_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the anchor point of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} ANCHOR {[x:float] [y:float]" + ANIMATION_SYNTAX);
	sink.para()->text(L"Changes the anchor point of the specified layer, or returns the current values if no arguments are given.");
	sink.para()
		->text(L"The anchor point is around which ")->see(L"MIXER FILL")->text(L" and ")->see(L"MIXER ROTATION")
		->text(L" will be done from.");
	sink.definitions()
		->item(L"x", L"The x anchor point, 0 = left edge of layer, 0.5 = middle of layer, 1.0 = right edge of layer. Higher and lower values allowed.")
		->item(L"y", L"The y anchor point, 0 = top edge of layer, 0.5 = middle of layer, 1.0 = bottom edge of layer. Higher and lower values allowed.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-10 ANCHOR 0.5 0.6 25 easeinsine", L"sets the anchor point");
	sink.example(
		L">> MIXER 1-10 ANCHOR\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.5 0.6", L"gets the anchor point");
}

std::wstring mixer_anchor_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto transform = get_current_transform(ctx).image_transform;
		auto anchor = transform.anchor;
		return L"201 MIXER OK\r\n"
			+ boost::lexical_cast<std::wstring>(anchor[0]) + L" "
			+ boost::lexical_cast<std::wstring>(anchor[1]) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 2 ? boost::lexical_cast<int>(ctx.parameters[2]) : 0;
	std::wstring tween = ctx.parameters.size() > 3 ? ctx.parameters[3] : L"linear";
	double x = boost::lexical_cast<double>(ctx.parameters.at(0));
	double y = boost::lexical_cast<double>(ctx.parameters.at(1));

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) mutable -> frame_transform
	{
		transform.image_transform.anchor[0] = x;
		transform.image_transform.anchor[1] = y;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_crop_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Crop a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} CROP {[left-edge:float] [top-edge:float] [right-edge:float] [bottom-edge:float]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Defines how a layer should be cropped before making other transforms via ")
		->see(L"MIXER FILL")->text(L", ")->see(L"MIXER ROTATION")->text(L" and ")->see(L"MIXER PERSPECTIVE")
		->text(L". See ")->see(L"MIXER CLIP")->text(L" if you want to change the viewport relative to the screen instead.");
	sink.definitions()
		->item(L"left-edge", L"A value between 0 and 1 defining how far into the layer to crop from the left edge.")
		->item(L"top-edge", L"A value between 0 and 1 defining how far into the layer to crop from the top edge.")
		->item(L"right-edge", L"A value between 1 and 0 defining how far into the layer to crop from the right edge.")
		->item(L"bottom-edge", L"A value between 1 and 0 defining how far into the layer to crop from the bottom edge.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 CROP 0.25 0.25 0.75 0.75 25 easeinsine", L"leaving a 25% crop around the edges");
	sink.example(
		L">> MIXER 1-0 CROP\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.25 0.25 0.75 0.75", L"for retrieving the current crop edges");
}

std::wstring mixer_crop_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto crop = get_current_transform(ctx).image_transform.crop;
		return L"201 MIXER OK\r\n"
			+ boost::lexical_cast<std::wstring>(crop.ul[0]) + L" "
			+ boost::lexical_cast<std::wstring>(crop.ul[1]) + L" "
			+ boost::lexical_cast<std::wstring>(crop.lr[0]) + L" "
			+ boost::lexical_cast<std::wstring>(crop.lr[1]) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 4 ? boost::lexical_cast<int>(ctx.parameters[4]) : 0;
	std::wstring tween = ctx.parameters.size() > 5 ? ctx.parameters[5] : L"linear";
	double ul_x = boost::lexical_cast<double>(ctx.parameters.at(0));
	double ul_y = boost::lexical_cast<double>(ctx.parameters.at(1));
	double lr_x = boost::lexical_cast<double>(ctx.parameters.at(2));
	double lr_y = boost::lexical_cast<double>(ctx.parameters.at(3));

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.crop.ul[0] = ul_x;
		transform.image_transform.crop.ul[1] = ul_y;
		transform.image_transform.crop.lr[0] = lr_x;
		transform.image_transform.crop.lr[1] = lr_y;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_rotation_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Rotate a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} ROTATION {[angle:float]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Returns or modifies the angle of which a layer is rotated by (clockwise degrees) ")
		->text(L"around the point specified by ")->see(L"MIXER ANCHOR")->text(L".");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 ROTATION 45 25 easeinsine");
	sink.example(
		L">> MIXER 1-0 ROTATION\n"
		L"<< 201 MIXER OK\n"
		L"<< 45", L"to retrieve the current angle");
}

std::wstring mixer_rotation_command(command_context& ctx)
{
	static const double PI = 3.141592653589793;

	return single_double_animatable_mixer_command(
			ctx,
			[](const frame_transform& t) { return t.image_transform.angle / PI * 180.0; },
			[](frame_transform& t, double value) { t.image_transform.angle = value * PI / 180.0; });
}

void mixer_perspective_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Adjust the perspective transform of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} PERSPECTIVE {[top-left-x:float] [top-left-y:float] [top-right-x:float] [top-right-y:float] [bottom-right-x:float] [bottom-right-y:float] [bottom-left-x:float] [bottom-left-y:float]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Perspective transforms (corner pins or distorts if you will) a layer.");
	sink.definitions()
		->item(L"top-left-x, top-left-y", L"Defines the x:y coordinate of the top left corner.")
		->item(L"top-right-x, top-right-y", L"Defines the x:y coordinate of the top right corner.")
		->item(L"bottom-right-x, bottom-right-y", L"Defines the x:y coordinate of the bottom right corner.")
		->item(L"bottom-left-x, bottom-left-y", L"Defines the x:y coordinate of the bottom left corner.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-10 PERSPECTIVE 0.4 0.4 0.6 0.4 1 1 0 1 25 easeinsine");
	sink.example(
		L">> MIXER 1-10 PERSPECTIVE\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.4 0.4 0.6 0.4 1 1 0 1", L"for retrieving the current corners");
}

std::wstring mixer_perspective_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto perspective = get_current_transform(ctx).image_transform.perspective;
		return
			L"201 MIXER OK\r\n"
			+ boost::lexical_cast<std::wstring>(perspective.ul[0]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.ul[1]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.ur[0]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.ur[1]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.lr[0]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.lr[1]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.ll[0]) + L" "
			+ boost::lexical_cast<std::wstring>(perspective.ll[1]) + L"\r\n";
	}

	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 8 ? boost::lexical_cast<int>(ctx.parameters[8]) : 0;
	std::wstring tween = ctx.parameters.size() > 9 ? ctx.parameters[9] : L"linear";
	double ul_x = boost::lexical_cast<double>(ctx.parameters.at(0));
	double ul_y = boost::lexical_cast<double>(ctx.parameters.at(1));
	double ur_x = boost::lexical_cast<double>(ctx.parameters.at(2));
	double ur_y = boost::lexical_cast<double>(ctx.parameters.at(3));
	double lr_x = boost::lexical_cast<double>(ctx.parameters.at(4));
	double lr_y = boost::lexical_cast<double>(ctx.parameters.at(5));
	double ll_x = boost::lexical_cast<double>(ctx.parameters.at(6));
	double ll_y = boost::lexical_cast<double>(ctx.parameters.at(7));

	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.perspective.ul[0] = ul_x;
		transform.image_transform.perspective.ul[1] = ul_y;
		transform.image_transform.perspective.ur[0] = ur_x;
		transform.image_transform.perspective.ur[1] = ur_y;
		transform.image_transform.perspective.lr[0] = lr_x;
		transform.image_transform.perspective.lr[1] = lr_y;
		transform.image_transform.perspective.ll[0] = ll_x;
		transform.image_transform.perspective.ll[1] = ll_y;
		return transform;
	}, duration, tween));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_mipmap_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Enable or disable mipmapping for a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} MIPMAP {[mipmap:0,1]|0}");
	sink.para()
		->text(L"Sets whether to use mipmapping (anisotropic filtering if supported) on a layer or not. ")
		->text(L"If no argument is given the current state is returned.");
	sink.para()->text(L"Mipmapping reduces aliasing when downscaling/perspective transforming.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-10 MIPMAP 1", L"for turning mipmapping on");
	sink.example(
		L">> MIXER 1-10 MIPMAP\n"
		L"<< 201 MIXER OK\n"
		L"<< 1", L"for getting the current state");
}

std::wstring mixer_mipmap_command(command_context& ctx)
{
	if (ctx.parameters.empty())
		return reply_value(ctx, [](const frame_transform& t) { return t.image_transform.use_mipmap ? 1 : 0; });

	transforms_applier transforms(ctx);
	bool value = boost::lexical_cast<int>(ctx.parameters.at(0));
	transforms.add(stage::transform_tuple_t(ctx.layer_index(), [=](frame_transform transform) -> frame_transform
	{
		transform.image_transform.use_mipmap = value;
		return transform;
	}, 0, L"linear"));
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_volume_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the volume of a layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]|-0} VOLUME {[volume:float]" + ANIMATION_SYNTAX);
	sink.para()->text(L"Changes the volume of the specified layer. The 1.0 is the original volume, which can be attenuated or amplified.");
	sink.para()->text(L"Retrieves the volume of the specified layer if no argument is given.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1-0 VOLUME 0 25 linear", L"for fading out the audio during 25 frames");
	sink.example(L">> MIXER 1-0 VOLUME 1.5", L"for amplifying the audio by 50%");
	sink.example(
		L">> MIXER 1-0 VOLUME\n"
		L"<< 201 MIXER OK\n"
		L"<< 0.8", L"to retrieve the current volume");
}

std::wstring mixer_volume_command(command_context& ctx)
{
	return single_double_animatable_mixer_command(
		ctx,
		[](const frame_transform& t) { return t.audio_transform.volume; },
		[](frame_transform& t, double value) { t.audio_transform.volume = value; });
}

void mixer_mastervolume_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Change the volume of an entire channel.");
	sink.syntax(L"MIXER [video_channel:int] MASTERVOLUME {[volume:float]}");
	sink.para()->text(L"Changes or retrieves (giving no argument) the volume of the entire channel.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1 MASTERVOLUME 0");
	sink.example(L">> MIXER 1 MASTERVOLUME 1");
	sink.example(L">> MIXER 1 MASTERVOLUME 0.5");
}

std::wstring mixer_mastervolume_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		auto volume = ctx.channel.channel->mixer().get_master_volume();
		return L"201 MIXER OK\r\n" + boost::lexical_cast<std::wstring>(volume)+L"\r\n";
	}

	float master_volume = boost::lexical_cast<float>(ctx.parameters.at(0));
	ctx.channel.channel->mixer().set_master_volume(master_volume);

	return L"202 MIXER OK\r\n";
}

void mixer_straight_alpha_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Turn straight alpha output on or off for a channel.");
	sink.syntax(L"MIXER [video_channel:int] STRAIGHT_ALPHA_OUTPUT {[straight_alpha:0,1|0]}");
	sink.para()->text(L"Turn straight alpha output on or off for the specified channel.");
	sink.para()->code(L"casparcg.config")->text(L" needs to be configured to enable the feature.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1 STRAIGHT_ALPHA_OUTPUT 0");
	sink.example(L">> MIXER 1 STRAIGHT_ALPHA_OUTPUT 1");
	sink.example(
			L">> MIXER 1 STRAIGHT_ALPHA_OUTPUT\n"
			L"<< 201 MIXER OK\n"
			L"<< 1");
}

std::wstring mixer_straight_alpha_command(command_context& ctx)
{
	if (ctx.parameters.empty())
	{
		bool state = ctx.channel.channel->mixer().get_straight_alpha_output();
		return L"201 MIXER OK\r\n" + boost::lexical_cast<std::wstring>(state) + L"\r\n";
	}

	bool state = boost::lexical_cast<bool>(ctx.parameters.at(0));
	ctx.channel.channel->mixer().set_straight_alpha_output(state);

	return L"202 MIXER OK\r\n";
}

void mixer_grid_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Create a grid of video layers.");
	sink.syntax(L"MIXER [video_channel:int] GRID [resolution:int]" + ANIMATION_SYNTAX);
	sink.para()
		->text(L"Creates a grid of video layer in ascending order of the layer index, i.e. if ")
		->code(L"resolution")->text(L" equals 2 then a 2x2 grid of layers will be created starting from layer 1.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1 GRID 2");
}

std::wstring mixer_grid_command(command_context& ctx)
{
	transforms_applier transforms(ctx);
	int duration = ctx.parameters.size() > 1 ? boost::lexical_cast<int>(ctx.parameters[1]) : 0;
	std::wstring tween = ctx.parameters.size() > 2 ? ctx.parameters[2] : L"linear";
	int n = boost::lexical_cast<int>(ctx.parameters.at(0));
	double delta = 1.0 / static_cast<double>(n);
	for (int x = 0; x < n; ++x)
	{
		for (int y = 0; y < n; ++y)
		{
			int index = x + y*n + 1;
			transforms.add(stage::transform_tuple_t(index, [=](frame_transform transform) -> frame_transform
			{
				transform.image_transform.fill_translation[0] = x*delta;
				transform.image_transform.fill_translation[1] = y*delta;
				transform.image_transform.fill_scale[0] = delta;
				transform.image_transform.fill_scale[1] = delta;
				transform.image_transform.clip_translation[0] = x*delta;
				transform.image_transform.clip_translation[1] = y*delta;
				transform.image_transform.clip_scale[0] = delta;
				transform.image_transform.clip_scale[1] = delta;
				return transform;
			}, duration, tween));
		}
	}
	transforms.apply();

	return L"202 MIXER OK\r\n";
}

void mixer_commit_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Commit all deferred mixer transforms.");
	sink.syntax(L"MIXER [video_channel:int] COMMIT");
	sink.para()->text(L"Commits all deferred mixer transforms on the specified channel. This ensures that all animations start at the same exact frame.");
	sink.para()->text(L"Examples:");
	sink.example(
		L">> MIXER 1-1 FILL 0 0 0.5 0.5 25 DEFER\n"
		L">> MIXER 1-2 FILL 0.5 0 0.5 0.5 25 DEFER\n"
		L">> MIXER 1 COMMIT");
}

std::wstring mixer_commit_command(command_context& ctx)
{
	transforms_applier transforms(ctx);
	transforms.commit_deferred();

	return L"202 MIXER OK\r\n";
}

void mixer_clear_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Clear all transformations on a channel or layer.");
	sink.syntax(L"MIXER [video_channel:int]{-[layer:int]} CLEAR");
	sink.para()->text(L"Clears all transformations on a channel or layer.");
	sink.para()->text(L"Examples:");
	sink.example(L">> MIXER 1 CLEAR", L"for clearing transforms on entire channel 1");
	sink.example(L">> MIXER 1-1 CLEAR", L"for clearing transforms on layer 1-1");
}

std::wstring mixer_clear_command(command_context& ctx)
{
	int layer = ctx.layer_id;

	if (layer == -1)
		ctx.channel.channel->stage().clear_transforms();
	else
		ctx.channel.channel->stage().clear_transforms(layer);

	return L"202 MIXER OK\r\n";
}

void channel_grid_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Open a new channel displaying a grid with the contents of all the existing channels.");
	sink.syntax(L"CHANNEL_GRID");
	sink.para()->text(L"Opens a new channel and displays a grid with the contents of all the existing channels.");
	sink.para()
		->text(L"The element ")->code(L"<channel-grid>true</channel-grid>")->text(L" must be present in ")
		->code(L"casparcg.config")->text(L" for this to work correctly.");
}

std::wstring channel_grid_command(command_context& ctx)
{
	int index = 1;
	auto self = ctx.channels.back();

	core::diagnostics::scoped_call_context save;
	core::diagnostics::call_context::for_thread().video_channel = ctx.channels.size();

	std::vector<std::wstring> params;
	params.push_back(L"SCREEN");
	params.push_back(L"0");
	params.push_back(L"NAME");
	params.push_back(L"Channel Grid Window");
	auto screen = ctx.consumer_registry->create_consumer(params, &self.channel->stage());

	self.channel->output().add(screen);

	for (auto& channel : ctx.channels)
	{
		if (channel.channel != self.channel)
		{
			core::diagnostics::call_context::for_thread().layer = index;
			auto producer = ctx.producer_registry->create_producer(get_producer_dependencies(self.channel, ctx), L"route://" + boost::lexical_cast<std::wstring>(channel.channel->index()));
			self.channel->stage().load(index, producer, false);
			self.channel->stage().play(index);
			index++;
		}
	}

	auto num_channels = ctx.channels.size() - 1;
	int square_side_length = std::ceil(std::sqrt(num_channels));

	ctx.channel_index = self.channel->index();
	ctx.channel = self;
	ctx.parameters.clear();
	ctx.parameters.push_back(boost::lexical_cast<std::wstring>(square_side_length));
	mixer_grid_command(ctx);

	return L"202 CHANNEL_GRID OK\r\n";
}

// Thumbnail Commands

void thumbnail_list_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"List all thumbnails.");
	sink.syntax(L"THUMBNAIL LIST");
	sink.para()->text(L"Lists all thumbnails.");
	sink.para()->text(L"Examples:");
	sink.example(
		L">> THUMBNAIL LIST\n"
		L"<< 200 THUMBNAIL LIST OK\n"
		L"<< \"AMB\" 20130301T124409 1149\n"
		L"<< \"foo/bar\" 20130523T234001 244");
}

std::wstring thumbnail_list_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"200 THUMBNAIL LIST OK\r\n";

	for (boost::filesystem::recursive_directory_iterator itr(env::thumbnails_folder()), end; itr != end; ++itr)
	{
		if (boost::filesystem::is_regular_file(itr->path()))
		{
			if (!boost::iequals(itr->path().extension().wstring(), L".png"))
				continue;

			auto relativePath = get_relative_without_extension(itr->path(), env::thumbnails_folder());
			auto str = relativePath.generic_wstring();

			if (str[0] == '\\' || str[0] == '/')
				str = std::wstring(str.begin() + 1, str.end());

			auto mtime = boost::filesystem::last_write_time(itr->path());
			auto mtime_readable = boost::posix_time::to_iso_wstring(boost::posix_time::from_time_t(mtime));
			auto file_size = boost::filesystem::file_size(itr->path());

			replyString << L"\"" << str << L"\" " << mtime_readable << L" " << file_size << L"\r\n";
		}
	}

	replyString << L"\r\n";

	return boost::to_upper_copy(replyString.str());
}

void thumbnail_retrieve_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Retrieve a thumbnail.");
	sink.syntax(L"THUMBNAIL RETRIEVE [filename:string]");
	sink.para()->text(L"Retrieves a thumbnail as a base64 encoded PNG-image.");
	sink.para()->text(L"Examples:");
	sink.example(
		L">> THUMBNAIL RETRIEVE foo/bar\n"
		L"<< 201 THUMBNAIL RETRIEVE OK\n"
		L"<< ...base64 data...");
}

std::wstring thumbnail_retrieve_command(command_context& ctx)
{
	std::wstring filename = env::thumbnails_folder();
	filename.append(ctx.parameters.at(0));
	filename.append(L".png");

	std::wstring file_contents;

	auto found_file = find_case_insensitive(filename);

	if (found_file)
		file_contents = read_file_base64(boost::filesystem::path(*found_file));

	if (file_contents.empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(filename + L" not found"));

	std::wstringstream reply;

	reply << L"201 THUMBNAIL RETRIEVE OK\r\n";
	reply << file_contents;
	reply << L"\r\n";
	return reply.str();
}

void thumbnail_generate_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Regenerate a thumbnail.");
	sink.syntax(L"THUMBNAIL GENERATE [filename:string]");
	sink.para()->text(L"Regenerates a thumbnail.");
}

std::wstring thumbnail_generate_command(command_context& ctx)
{
	if (ctx.thumb_gen)
	{
		ctx.thumb_gen->generate(ctx.parameters.at(0));
		return L"202 THUMBNAIL GENERATE OK\r\n";
	}
	else
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"Thumbnail generation turned off"));
}

void thumbnail_generateall_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Regenerate all thumbnails.");
	sink.syntax(L"THUMBNAIL GENERATE_ALL");
	sink.para()->text(L"Regenerates all thumbnails.");
}

std::wstring thumbnail_generateall_command(command_context& ctx)
{
	if (ctx.thumb_gen)
	{
		ctx.thumb_gen->generate_all();
		return L"202 THUMBNAIL GENERATE_ALL OK\r\n";
	}
	else
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info(L"Thumbnail generation turned off"));
}

// Query Commands

void cinf_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get information about a media file.");
	sink.syntax(L"CINF [filename:string]");
	sink.para()->text(L"Returns information about a media file.");
}

std::wstring cinf_command(command_context& ctx)
{
	std::wstring info;
	for (boost::filesystem::recursive_directory_iterator itr(env::media_folder()), end; itr != end && info.empty(); ++itr)
	{
		auto path = itr->path();
		auto file = path.replace_extension(L"").filename().wstring();
		if (boost::iequals(file, ctx.parameters.at(0)))
			info += MediaInfo(itr->path(), ctx.media_info_repo);
	}

	if (info.empty())
		CASPAR_THROW_EXCEPTION(file_not_found());

	std::wstringstream replyString;
	replyString << L"200 CINF OK\r\n";
	replyString << info << "\r\n";

	return replyString.str();
}

void cls_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"List all media files.");
	sink.syntax(L"CLS");
	sink.para()
		->text(L"Lists all media files in the ")->code(L"media")->text(L" folder. Use the command ")
		->see(L"INFO PATHS")->text(L" to get the path to the ")->code(L"media")->text(L" folder.");
}

std::wstring cls_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"200 CLS OK\r\n";
	replyString << ListMedia(ctx.media_info_repo);
	replyString << L"\r\n";
	return boost::to_upper_copy(replyString.str());
}

void fls_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"List all fonts.");
	sink.syntax(L"FLS");
	sink.para()
		->text(L"Lists all font files in the ")->code(L"fonts")->text(L" folder. Use the command ")
		->see(L"INFO PATHS")->text(L" to get the path to the ")->code(L"fonts")->text(L" folder.");
	sink.para()->text(L"Columns in order from left to right are: Font name and font path.");
}

std::wstring fls_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"200 FLS OK\r\n";

	for (auto& font : core::text::list_fonts())
		replyString << L"\"" << font.first << L"\" \"" << get_relative(font.second, env::font_folder()).wstring() << L"\"\r\n";

	replyString << L"\r\n";

	return replyString.str();
}

void tls_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"List all templates.");
	sink.syntax(L"TLS");
	sink.para()
		->text(L"Lists all template files in the ")->code(L"templates")->text(L" folder. Use the command ")
		->see(L"INFO PATHS")->text(L" to get the path to the ")->code(L"templates")->text(L" folder.");
}

std::wstring tls_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"200 TLS OK\r\n";

	replyString << ListTemplates(ctx.cg_registry);
	replyString << L"\r\n";

	return replyString.str();
}

void version_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get version information.");
	sink.syntax(L"VERSION {[component:string]}");
	sink.para()->text(L"Returns the version of specified component.");
	sink.para()->text(L"Examples:");
	sink.example(
		L">> VERSION\n"
		L"<< 201 VERSION OK\n"
		L"<< 2.1.0.f207a33 STABLE");
	sink.example(
		L">> VERSION SERVER\n"
		L"<< 201 VERSION OK\n"
		L"<< 2.1.0.f207a33 STABLE");
	sink.example(
		L">> VERSION FLASH\n"
		L"<< 201 VERSION OK\n"
		L"<< 11.8.800.94");
}

std::wstring version_command(command_context& ctx)
{
	if (!ctx.parameters.empty() && !boost::iequals(ctx.parameters.at(0), L"SERVER"))
	{
		auto version = ctx.system_info_repo->get_version(ctx.parameters.at(0));

		return L"201 VERSION OK\r\n" + version + L"\r\n";
	}

	return L"201 VERSION OK\r\n" + env::version() + L"\r\n";
}

void info_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get a list of the available channels.");
	sink.syntax(L"INFO");
	sink.para()->text(L"Retrieves a list of the available channels.");
	sink.example(
		L">> INFO\n"
		L"<< 200 INFO OK\n"
		L"<< 1 720p5000 PLAYING\n"
		L"<< 2 PAL PLAYING");
}

std::wstring info_command(command_context& ctx)
{
	std::wstringstream replyString;
	// This is needed for backwards compatibility with old clients
	replyString << L"200 INFO OK\r\n";
	for (size_t n = 0; n < ctx.channels.size(); ++n)
		replyString << n + 1 << L" " << ctx.channels.at(n).channel->video_format_desc().name << L" PLAYING\r\n";
	replyString << L"\r\n";
	return replyString.str();
}

std::wstring create_info_xml_reply(const boost::property_tree::wptree& info, std::wstring command = L"")
{
	std::wstringstream replyString;

	if (command.empty())
		replyString << L"201 INFO OK\r\n";
	else
		replyString << L"201 INFO " << std::move(command) << L" OK\r\n";

	boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);
	boost::property_tree::xml_parser::write_xml(replyString, info, w);
	replyString << L"\r\n";
	return replyString.str();
}

void info_channel_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get information about a channel or a layer.");
	sink.syntax(L"INFO [video_channel:int]{-[layer:int]}");
	sink.para()->text(L"Get information about a channel or a specific layer on a channel.");
	sink.para()->text(L"If ")->code(L"layer")->text(L" is ommitted information about the whole channel is returned.");
}

std::wstring info_channel_command(command_context& ctx)
{
	boost::property_tree::wptree info;
	int layer = ctx.layer_index(std::numeric_limits<int>::min());

	if (layer == std::numeric_limits<int>::min())
	{
		info.add_child(L"channel", ctx.channel.channel->info())
			.add(L"index", ctx.channel_index);
	}
	else
	{
		if (ctx.parameters.size() >= 1)
		{
			if (boost::iequals(ctx.parameters.at(0), L"B"))
				info.add_child(L"producer", ctx.channel.channel->stage().background(layer).get()->info());
			else
				info.add_child(L"producer", ctx.channel.channel->stage().foreground(layer).get()->info());
		}
		else
		{
			info.add_child(L"layer", ctx.channel.channel->stage().info(layer).get()).add(L"index", layer);
		}
	}

	return create_info_xml_reply(info);
}

void info_template_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get information about a template.");
	sink.syntax(L"INFO TEMPLATE [template:string]");
	sink.para()->text(L"Gets information about the specified template.");
}

std::wstring info_template_command(command_context& ctx)
{
	auto filename = ctx.parameters.at(0);

	std::wstringstream str;
	str << u16(ctx.cg_registry->read_meta_info(filename));
	boost::property_tree::wptree info;
	boost::property_tree::xml_parser::read_xml(str, info, boost::property_tree::xml_parser::trim_whitespace | boost::property_tree::xml_parser::no_comments);

	return create_info_xml_reply(info, L"TEMPLATE");
}

void info_config_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get the contents of the configuration used.");
	sink.syntax(L"INFO CONFIG");
	sink.para()->text(L"Gets the contents of the configuration used.");
}

std::wstring info_config_command(command_context& ctx)
{
	return create_info_xml_reply(caspar::env::properties(), L"CONFIG");
}

void info_paths_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get information about the paths used.");
	sink.syntax(L"INFO PATHS");
	sink.para()->text(L"Gets information about the paths used.");
}

std::wstring info_paths_command(command_context& ctx)
{
	boost::property_tree::wptree info;
	info.add_child(L"paths", caspar::env::properties().get_child(L"configuration.paths"));
	info.add(L"paths.initial-path", boost::filesystem::initial_path().wstring() + L"/");

	return create_info_xml_reply(info, L"PATHS");
}

void info_system_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get system information.");
	sink.syntax(L"INFO SYSTEM");
	sink.para()->text(L"Gets system information like OS, CPU and library version numbers.");
}

std::wstring info_system_command(command_context& ctx)
{
	boost::property_tree::wptree info;

	info.add(L"system.name", caspar::system_product_name());
	info.add(L"system.os.description", caspar::os_description());
	info.add(L"system.cpu", caspar::cpu_info());

	ctx.system_info_repo->fill_information(info);

	return create_info_xml_reply(info, L"SYSTEM");
}

void info_server_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get detailed information about all channels.");
	sink.syntax(L"INFO SERVER");
	sink.para()->text(L"Gets detailed information about all channels.");
}

std::wstring info_server_command(command_context& ctx)
{
	boost::property_tree::wptree info;

	int index = 0;
	for (auto& channel : ctx.channels)
		info.add_child(L"channels.channel", channel.channel->info())
				.add(L"index", ++index);

	return create_info_xml_reply(info, L"SERVER");
}

void info_queues_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get detailed information about all AMCP Command Queues.");
	sink.syntax(L"INFO QUEUES");
	sink.para()->text(L"Gets detailed information about all AMCP Command Queues.");
}

std::wstring info_queues_command(command_context& ctx)
{
	return create_info_xml_reply(AMCPCommandQueue::info_all_queues(), L"QUEUES");
}

void info_threads_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Lists all known threads in the server.");
	sink.syntax(L"INFO THREADS");
	sink.para()->text(L"Lists all known threads in the server.");
}

std::wstring info_threads_command(command_context& ctx)
{
	std::wstringstream replyString;
	replyString << L"200 INFO THREADS OK\r\n";

	for (auto& thread : get_thread_infos())
	{
		replyString << thread->native_id << L" " << u16(thread->name) << L"\r\n";
	}

	replyString << L"\r\n";
	return replyString.str();
}

void info_delay_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get the current delay on a channel or a layer.");
	sink.syntax(L"INFO [video_channel:int]{-[layer:int]} DELAY");
	sink.para()->text(L"Get the current delay on the specified channel or layer.");
}

std::wstring info_delay_command(command_context& ctx)
{
	boost::property_tree::wptree info;
	auto layer = ctx.layer_index(std::numeric_limits<int>::min());

	if (layer == std::numeric_limits<int>::min())
		info.add_child(L"channel-delay", ctx.channel.channel->delay_info());
	else
		info.add_child(L"layer-delay", ctx.channel.channel->stage().delay_info(layer).get())
			.add(L"index", layer);

	return create_info_xml_reply(info, L"DELAY");
}

void diag_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Open the diagnostics window.");
	sink.syntax(L"DIAG");
	sink.para()->text(L"Opens the ")->see(L"Diagnostics Window")->text(L".");
}

std::wstring diag_command(command_context& ctx)
{
	core::diagnostics::osd::show_graphs(true);

	return L"202 DIAG OK\r\n";
}

void gl_info_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Get information about the allocated and pooled OpenGL resources.");
	sink.syntax(L"GL INFO");
	sink.para()->text(L"Retrieves information about the allocated and pooled OpenGL resources.");
}

std::wstring gl_info_command(command_context& ctx)
{
	auto device = ctx.ogl_device;

	if (!device)
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info("GL command only supported with OpenGL accelerator."));

	std::wstringstream result;
	result << L"201 GL INFO OK\r\n";

	boost::property_tree::xml_writer_settings<std::wstring> w(' ', 3);
	auto info = device->info();
	boost::property_tree::write_xml(result, info, w);
	result << L"\r\n";

	return result.str();
}

void gl_gc_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Release pooled OpenGL resources.");
	sink.syntax(L"GL GC");
	sink.para()->text(L"Releases all the pooled OpenGL resources. ")->strong(L"May cause a pause on all video channels.");
}

std::wstring gl_gc_command(command_context& ctx)
{
	auto device = ctx.ogl_device;

	if (!device)
		CASPAR_THROW_EXCEPTION(not_supported() << msg_info("GL command only supported with OpenGL accelerator."));

	device->gc().wait();

	return L"202 GL GC OK\r\n";
}

static const int WIDTH = 80;

struct max_width_sink : public core::help_sink
{
	std::size_t max_width = 0;

	void begin_item(const std::wstring& name) override
	{
		max_width = std::max(name.length(), max_width);
	};
};

struct short_description_sink : public core::help_sink
{
	std::size_t width;
	std::wstringstream& out;

	short_description_sink(std::size_t width, std::wstringstream& out) : width(width), out(out) { }

	void begin_item(const std::wstring& name) override
	{
		out << std::left << std::setw(width + 1) << name;
	};

	void short_description(const std::wstring& short_description) override
	{
		out << short_description << L"\r\n";
	};
};

struct simple_paragraph_builder : core::paragraph_builder
{
	std::wostringstream out;
	std::wstringstream& commit_to;

	simple_paragraph_builder(std::wstringstream& out) : commit_to(out) { }
	~simple_paragraph_builder()
	{
		commit_to << core::wordwrap(out.str(), WIDTH) << L"\n";
	}
	spl::shared_ptr<paragraph_builder> text(std::wstring text) override
	{
		out << std::move(text);
		return shared_from_this();
	}
	spl::shared_ptr<paragraph_builder> code(std::wstring txt) override { return text(std::move(txt)); }
	spl::shared_ptr<paragraph_builder> strong(std::wstring item) override { return text(L"*" + std::move(item) + L"*"); }
	spl::shared_ptr<paragraph_builder> see(std::wstring item) override { return text(std::move(item)); }
	spl::shared_ptr<paragraph_builder> url(std::wstring url, std::wstring name)  override { return text(std::move(url)); }
};

struct simple_definition_list_builder : core::definition_list_builder
{
	std::wstringstream& out;

	simple_definition_list_builder(std::wstringstream& out) : out(out) { }
	~simple_definition_list_builder()
	{
		out << L"\n";
	}

	spl::shared_ptr<definition_list_builder> item(std::wstring term, std::wstring description) override
	{
		out << core::indent(core::wordwrap(term, WIDTH - 2), L"  ");
		out << core::indent(core::wordwrap(description, WIDTH - 4), L"    ");
		return shared_from_this();
	}
};

struct long_description_sink : public core::help_sink
{
	std::wstringstream& out;

	long_description_sink(std::wstringstream& out) : out(out) { }

	void syntax(const std::wstring& syntax) override
	{
		out << L"Syntax:\n";
		out << core::indent(core::wordwrap(syntax, WIDTH - 2), L"  ") << L"\n";
	};

	spl::shared_ptr<core::paragraph_builder> para() override
	{
		return spl::make_shared<simple_paragraph_builder>(out);
	}

	spl::shared_ptr<core::definition_list_builder> definitions() override
	{
		return spl::make_shared<simple_definition_list_builder>(out);
	}

	void example(const std::wstring& code, const std::wstring& caption) override
	{
		out << core::indent(core::wordwrap(code, WIDTH - 2), L"  ");

		if (!caption.empty())
			out << core::indent(core::wordwrap(L"..." + caption, WIDTH - 2), L"  ");

		out << L"\n";
	}
private:
	void begin_item(const std::wstring& name) override
	{
		out << name << L"\n\n";
	};
};

std::wstring create_help_list(const std::wstring& help_command, const command_context& ctx, std::set<std::wstring> tags)
{
	std::wstringstream result;
	result << L"200 " << help_command << L" OK\r\n";
	max_width_sink width;
	ctx.help_repo->help(tags, width);
	short_description_sink sink(width.max_width, result);
	sink.width = width.max_width;
	ctx.help_repo->help(tags, sink);
	result << L"\r\n";
	return result.str();
}

std::wstring create_help_details(const std::wstring& help_command, const command_context& ctx, std::set<std::wstring> tags)
{
	std::wstringstream result;
	result << L"201 " << help_command << L" OK\r\n";
	auto joined = boost::join(ctx.parameters, L" ");
	long_description_sink sink(result);
	ctx.help_repo->help(tags, joined, sink);
	result << L"\r\n";
	return result.str();
}

void help_describer(core::help_sink& sink, const core::help_repository& repository)
{
	sink.short_description(L"Show online help for AMCP commands.");
	sink.syntax(LR"(HELP {[command:string]})");
	sink.para()->text(LR"(Shows online help for a specific command or a list of all commands.)");
	sink.example(L">> HELP", L"Shows a list of commands.");
	sink.example(L">> HELP PLAY", L"Shows a detailed description of the PLAY command.");
}

std::wstring help_command(command_context& ctx)
{
	if (ctx.parameters.size() == 0)
		return create_help_list(L"HELP", ctx, { L"AMCP" });
	else
		return create_help_details(L"HELP", ctx, { L"AMCP" });
}

void help_producer_describer(core::help_sink& sink, const core::help_repository& repository)
{
	sink.short_description(L"Show online help for producers.");
	sink.syntax(LR"(HELP PRODUCER {[producer:string]})");
	sink.para()->text(LR"(Shows online help for a specific producer or a list of all producers.)");
	sink.example(L">> HELP PRODUCER", L"Shows a list of producers.");
	sink.example(L">> HELP PRODUCER FFmpeg Producer", L"Shows a detailed description of the FFmpeg Producer.");
}

std::wstring help_producer_command(command_context& ctx)
{
	if (ctx.parameters.size() == 0)
		return create_help_list(L"HELP PRODUCER", ctx, { L"producer" });
	else
		return create_help_details(L"HELP PRODUCER", ctx, { L"producer" });
}

void help_consumer_describer(core::help_sink& sink, const core::help_repository& repository)
{
	sink.short_description(L"Show online help for consumers.");
	sink.syntax(LR"(HELP CONSUMER {[consumer:string]})");
	sink.para()->text(LR"(Shows online help for a specific consumer or a list of all consumers.)");
	sink.example(L">> HELP CONSUMER", L"Shows a list of consumers.");
	sink.example(L">> HELP CONSUMER Decklink Consumer", L"Shows a detailed description of the Decklink Consumer.");
}

std::wstring help_consumer_command(command_context& ctx)
{
	if (ctx.parameters.size() == 0)
		return create_help_list(L"HELP CONSUMER", ctx, { L"consumer" });
	else
		return create_help_details(L"HELP CONSUMER", ctx, { L"consumer" });
}

void bye_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Disconnect the session.");
	sink.syntax(L"BYE");
	sink.para()
		->text(L"Disconnects from the server if connected remotely, if interacting directly with the console ")
		->text(L"on the machine Caspar is running on then this will achieve the same as the ")->see(L"KILL")->text(L" command.");
}

std::wstring bye_command(command_context& ctx)
{
	ctx.client->disconnect();
	return L"";
}

void kill_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Shutdown the server.");
	sink.syntax(L"KILL");
	sink.para()->text(L"Shuts the server down.");
}

std::wstring kill_command(command_context& ctx)
{
	ctx.shutdown_server_now.set_value(false);	//false for not attempting to restart
	return L"202 KILL OK\r\n";
}

void restart_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Shutdown the server with restart exit code.");
	sink.syntax(L"RESTART");
	sink.para()
		->text(L"Shuts the server down, but exits with return code 5 instead of 0. ")
		->text(L"Intended for use in combination with ")->code(L"casparcg_auto_restart.bat");
}

std::wstring restart_command(command_context& ctx)
{
	ctx.shutdown_server_now.set_value(true);	//true for attempting to restart
	return L"202 RESTART OK\r\n";
}

void lock_describer(core::help_sink& sink, const core::help_repository& repo)
{
	sink.short_description(L"Lock or unlock access to a channel.");
	sink.syntax(L"LOCK [video_channel:int] [action:ACQUIRE,RELEASE,CLEAR] {[lock-phrase:string]}");
	sink.para()->text(L"Allows for exclusive access to a channel.");
	sink.para()->text(L"Examples:");
	sink.example(L"LOCK 1 ACQUIRE secret");
	sink.example(L"LOCK 1 RELEASE");
	sink.example(L"LOCK 1 CLEAR");
}

std::wstring lock_command(command_context& ctx)
{
	int channel_index = boost::lexical_cast<int>(ctx.parameters.at(0)) - 1;
	auto lock = ctx.channels.at(channel_index).lock;
	auto command = boost::to_upper_copy(ctx.parameters.at(1));

	if (command == L"ACQUIRE")
	{
		std::wstring lock_phrase = ctx.parameters.at(2);

		//TODO: read options

		//just lock one channel
		if (!lock->try_lock(lock_phrase, ctx.client))
			return L"503 LOCK ACQUIRE FAILED\r\n";

		return L"202 LOCK ACQUIRE OK\r\n";
	}
	else if (command == L"RELEASE")
	{
		lock->release_lock(ctx.client);
		return L"202 LOCK RELEASE OK\r\n";
	}
	else if (command == L"CLEAR")
	{
		std::wstring override_phrase = env::properties().get(L"configuration.lock-clear-phrase", L"");
		std::wstring client_override_phrase;

		if (!override_phrase.empty())
			client_override_phrase = ctx.parameters.at(2);

		//just clear one channel
		if (client_override_phrase != override_phrase)
			return L"503 LOCK CLEAR FAILED\r\n";

		lock->clear_locks();

		return L"202 LOCK CLEAR OK\r\n";
	}

	CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"Unknown LOCK command " + command));
}

void register_commands(amcp_command_repository& repo)
{
	repo.register_channel_command(	L"Basic Commands",		L"LOADBG",						loadbg_describer,					loadbg_command,					1);
	repo.register_channel_command(	L"Basic Commands",		L"LOAD",						load_describer,						load_command,					1);
	repo.register_channel_command(	L"Basic Commands",		L"PLAY",						play_describer,						play_command,					0);
	repo.register_channel_command(	L"Basic Commands",		L"PAUSE",						pause_describer,					pause_command,					0);
	repo.register_channel_command(	L"Basic Commands",		L"RESUME",						resume_describer,					resume_command,					0);
	repo.register_channel_command(	L"Basic Commands",		L"STOP",						stop_describer,						stop_command,					0);
	repo.register_channel_command(	L"Basic Commands",		L"CLEAR",						clear_describer,					clear_command,					0);
	repo.register_channel_command(	L"Basic Commands",		L"CALL",						call_describer,						call_command,					1);
	repo.register_channel_command(	L"Basic Commands",		L"SWAP",						swap_describer,						swap_command,					1);
	repo.register_channel_command(	L"Basic Commands",		L"ADD",							add_describer,						add_command,					1);
	repo.register_channel_command(	L"Basic Commands",		L"REMOVE",						remove_describer,					remove_command,					0);
	repo.register_channel_command(	L"Basic Commands",		L"PRINT",						print_describer,					print_command,					0);
	repo.register_command(			L"Basic Commands",		L"LOG LEVEL",					log_level_describer,				log_level_command,				1);
	repo.register_channel_command(	L"Basic Commands",		L"SET",							set_describer,						set_command,					2);
	repo.register_command(			L"Basic Commands",		L"LOCK",						lock_describer,						lock_command,					2);

	repo.register_command(			L"Data Commands", 		L"DATA STORE",					data_store_describer,				data_store_command,				2);
	repo.register_command(			L"Data Commands", 		L"DATA RETRIEVE",				data_retrieve_describer,			data_retrieve_command,			1);
	repo.register_command(			L"Data Commands", 		L"DATA LIST",					data_list_describer,				data_list_command,				0);
	repo.register_command(			L"Data Commands", 		L"DATA REMOVE",					data_remove_describer,				data_remove_command,			1);

	repo.register_channel_command(	L"Template Commands",	L"CG ADD",						cg_add_describer,					cg_add_command,					3);
	repo.register_channel_command(	L"Template Commands",	L"CG PLAY",						cg_play_describer,					cg_play_command,				1);
	repo.register_channel_command(	L"Template Commands",	L"CG STOP",						cg_stop_describer,					cg_stop_command,				1);
	repo.register_channel_command(	L"Template Commands",	L"CG NEXT",						cg_next_describer,					cg_next_command,				1);
	repo.register_channel_command(	L"Template Commands",	L"CG REMOVE",					cg_remove_describer,				cg_remove_command,				1);
	repo.register_channel_command(	L"Template Commands",	L"CG CLEAR",					cg_clear_describer,					cg_clear_command,				0);
	repo.register_channel_command(	L"Template Commands",	L"CG UPDATE",					cg_update_describer,				cg_update_command,				2);
	repo.register_channel_command(	L"Template Commands",	L"CG INVOKE",					cg_invoke_describer,				cg_invoke_command,				2);
	repo.register_channel_command(	L"Template Commands",	L"CG INFO",						cg_info_describer,					cg_info_command,				0);

	repo.register_channel_command(	L"Mixer Commands",		L"MIXER KEYER",					mixer_keyer_describer,				mixer_keyer_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER CHROMA",				mixer_chroma_describer,				mixer_chroma_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER BLEND",					mixer_blend_describer,				mixer_blend_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER OPACITY",				mixer_opacity_describer,			mixer_opacity_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER BRIGHTNESS",			mixer_brightness_describer,			mixer_brightness_command,		0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER SATURATION",			mixer_saturation_describer,			mixer_saturation_command,		0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER CONTRAST",				mixer_contrast_describer,			mixer_contrast_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER LEVELS",				mixer_levels_describer,				mixer_levels_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER FILL",					mixer_fill_describer,				mixer_fill_command,				0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER CLIP",					mixer_clip_describer,				mixer_clip_command,				0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER ANCHOR",				mixer_anchor_describer,				mixer_anchor_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER CROP",					mixer_crop_describer,				mixer_crop_command,				0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER ROTATION",				mixer_rotation_describer,			mixer_rotation_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER PERSPECTIVE",			mixer_perspective_describer,		mixer_perspective_command,		0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER MIPMAP",				mixer_mipmap_describer,				mixer_mipmap_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER VOLUME",				mixer_volume_describer,				mixer_volume_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER MASTERVOLUME",			mixer_mastervolume_describer,		mixer_mastervolume_command,		0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER STRAIGHT_ALPHA_OUTPUT",	mixer_straight_alpha_describer,		mixer_straight_alpha_command,	0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER GRID",					mixer_grid_describer,				mixer_grid_command,				1);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER COMMIT",				mixer_commit_describer,				mixer_commit_command,			0);
	repo.register_channel_command(	L"Mixer Commands",		L"MIXER CLEAR",					mixer_clear_describer,				mixer_clear_command,			0);
	repo.register_command(			L"Mixer Commands",		L"CHANNEL_GRID",				channel_grid_describer,				channel_grid_command,			0);

	repo.register_command(			L"Thumbnail Commands",	L"THUMBNAIL LIST",				thumbnail_list_describer,			thumbnail_list_command,			0);
	repo.register_command(			L"Thumbnail Commands",	L"THUMBNAIL RETRIEVE",			thumbnail_retrieve_describer,		thumbnail_retrieve_command,		1);
	repo.register_command(			L"Thumbnail Commands",	L"THUMBNAIL GENERATE",			thumbnail_generate_describer,		thumbnail_generate_command,		1);
	repo.register_command(			L"Thumbnail Commands",	L"THUMBNAIL GENERATE_ALL",		thumbnail_generateall_describer,	thumbnail_generateall_command,	0);

	repo.register_command(			L"Query Commands",		L"CINF",						cinf_describer,						cinf_command,					1);
	repo.register_command(			L"Query Commands",		L"CLS",							cls_describer,						cls_command,					0);
	repo.register_command(			L"Query Commands",		L"FLS",							fls_describer,						fls_command,					0);
	repo.register_command(			L"Query Commands",		L"TLS",							tls_describer,						tls_command,					0);
	repo.register_command(			L"Query Commands",		L"VERSION",						version_describer,					version_command,				0);
	repo.register_command(			L"Query Commands",		L"INFO",						info_describer,						info_command,					0);
	repo.register_channel_command(	L"Query Commands",		L"INFO",						info_channel_describer,				info_channel_command,			0);
	repo.register_command(			L"Query Commands",		L"INFO TEMPLATE",				info_template_describer,			info_template_command,			1);
	repo.register_command(			L"Query Commands",		L"INFO CONFIG",					info_config_describer,				info_config_command,			0);
	repo.register_command(			L"Query Commands",		L"INFO PATHS",					info_paths_describer,				info_paths_command,				0);
	repo.register_command(			L"Query Commands",		L"INFO SYSTEM",					info_system_describer,				info_system_command,			0);
	repo.register_command(			L"Query Commands",		L"INFO SERVER",					info_server_describer,				info_server_command,			0);
	repo.register_command(			L"Query Commands",		L"INFO QUEUES",					info_queues_describer,				info_queues_command,			0);
	repo.register_command(			L"Query Commands",		L"INFO THREADS",				info_threads_describer,				info_threads_command,			0);
	repo.register_channel_command(	L"Query Commands",		L"INFO DELAY",					info_delay_describer,				info_delay_command,				0);
	repo.register_command(			L"Query Commands",		L"DIAG",						diag_describer,						diag_command,					0);
	repo.register_command(			L"Query Commands",		L"GL INFO",						gl_info_describer,					gl_info_command,				0);
	repo.register_command(			L"Query Commands",		L"GL GC",						gl_gc_describer,					gl_gc_command,					0);
	repo.register_command(			L"Query Commands",		L"BYE",							bye_describer,						bye_command,					0);
	repo.register_command(			L"Query Commands",		L"KILL",						kill_describer,						kill_command,					0);
	repo.register_command(			L"Query Commands",		L"RESTART",						restart_describer,					restart_command,				0);
	repo.register_command(			L"Query Commands",		L"HELP",						help_describer,						help_command,					0);
	repo.register_command(			L"Query Commands",		L"HELP PRODUCER",				help_producer_describer,			help_producer_command,			0);
	repo.register_command(			L"Query Commands",		L"HELP CONSUMER",				help_consumer_describer,			help_consumer_command,			0);
}

}	//namespace amcp
}}	//namespace caspar
