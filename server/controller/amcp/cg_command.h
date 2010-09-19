#pragma once

#include "../../renderer/renderer_fwd.h"

#include <boost/fusion/container/vector.hpp>

#include <functional>
#include <string>

namespace caspar { namespace controller { namespace amcp {
	
struct channel_cg_add_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_remove_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_clear_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_play_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_stop_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_next_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_goto_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_update_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_cg_invoke_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

}}}
