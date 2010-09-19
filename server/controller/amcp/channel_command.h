#pragma once

#include "../../renderer/renderer_fwd.h"

#include <boost/fusion/container/vector.hpp>
#include <boost/regex.hpp>

#include <functional>
#include <string>

namespace caspar { namespace controller { namespace amcp {
	
const unsigned int DEFAULT_CHANNEL_LAYER = -2;

struct channel_info
{
	int channel_index;
	renderer::render_device_ptr channel;
	int layer_index;

	static channel_info parse(boost::wsmatch& what, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_info_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_load_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_play_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_stop_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

struct channel_clear_command
{
	static std::function<std::wstring()> parse(const std::wstring& message, const std::vector<renderer::render_device_ptr>& channels);
};

}}}
