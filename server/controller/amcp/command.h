#pragma once

#include "../../renderer/renderer_fwd.h"

#define FUSION_MAX_VECTOR_SIZE 25

#include <boost/fusion/container/vector.hpp>

#include <functional>
#include <string>

#include "channel_command.h"
#include "cg_command.h"


namespace caspar { namespace controller { namespace amcp {

struct list_media_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct list_template_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct media_info_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct template_info_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct version_info_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

typedef boost::fusion::vector
<
	list_media_command,
	list_template_command,
	media_info_command,
	template_info_command,
	version_info_command,
	channel_info_command,
	channel_load_command,
	channel_play_command,
	channel_stop_command,
	channel_clear_command,
	channel_cg_add_command,
	channel_cg_add_command,
	channel_cg_remove_command,
	channel_cg_clear_command,
	channel_cg_play_command,
	channel_cg_stop_command,
	channel_cg_next_command,
	channel_cg_goto_command,
	channel_cg_update_command,
	channel_cg_invoke_command
> command_list;

}}}
