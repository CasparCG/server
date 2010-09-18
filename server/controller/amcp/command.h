#pragma once

#include "../../renderer/renderer_fwd.h"

#include <boost/fusion/container/vector.hpp>

#include <functional>
#include <string>

namespace caspar { namespace controller { namespace protocol { namespace amcp {
	
const unsigned int DEFAULT_CHANNEL_LAYER = 0;

struct channel_info_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct load_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct play_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct stop_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct clear_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct list_media_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct list_template_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct media_info_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct template_info_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct version_info_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct disconnect_command
{
	static std::function<void()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

typedef boost::fusion::vector
<
	load_command,
	play_command,
	stop_command,
	clear_command,
	list_media_command,
	list_template_command,
	media_info_command,
	template_info_command,
	version_info_command,
	disconnect_command
> command_list;

}}}}
