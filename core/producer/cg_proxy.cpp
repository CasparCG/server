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
* Author: Helge Norberg, helge.norberg@svt.se
*/

#include "../StdAfx.h"

#include "cg_proxy.h"

#include "frame_producer.h"
#include "stage.h"
#include "../video_channel.h"
#include "../diagnostics/call_context.h"

#include <common/env.h>
#include <common/os/filesystem.h>

#include <boost/thread/mutex.hpp>
#include <boost/thread/lock_guard.hpp>
#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>

#include <future>
#include <map>

namespace caspar { namespace core {

const spl::shared_ptr<cg_proxy>& cg_proxy::empty()
{
	class empty_proxy : public cg_proxy
	{
		void add(int, const std::wstring&, bool, const std::wstring&, const std::wstring&) override {}
		void remove(int) override {}
		void play(int) override {}
		void stop(int, unsigned int) override {}
		void next(int) override {}
		void update(int, const std::wstring&) override {}
		std::wstring invoke(int, const std::wstring&) override { return L""; }
		std::wstring description(int) override { return L"empty cg producer"; }
		std::wstring template_host_info() override { return L"empty cg producer"; }
	};

	static spl::shared_ptr<cg_proxy> instance = spl::make_shared<empty_proxy>();
	return instance;
}

using namespace boost::multi_index;

struct cg_producer_registry::impl
{
private:
	struct record
	{
		std::wstring			name;
		meta_info_extractor		info_extractor;
		cg_proxy_factory		proxy_factory;
		cg_producer_factory		producer_factory;
		bool					reusable_producer_instance;
	};

	struct name {};
	struct extension {};

	mutable boost::mutex			mutex_;
	std::map<std::wstring, record>	records_by_extension_;
public:
	void register_cg_producer(
			std::wstring cg_producer_name,
			std::set<std::wstring> file_extensions,
			meta_info_extractor info_extractor,
			cg_proxy_factory proxy_factory,
			cg_producer_factory producer_factory,
			bool reusable_producer_instance)
	{
		boost::lock_guard<boost::mutex> lock(mutex_);

		record rec
		{
			std::move(cg_producer_name),
			std::move(info_extractor),
			std::move(proxy_factory),
			std::move(producer_factory),
			reusable_producer_instance
		};

		for (auto& extension : file_extensions)
		{
			records_by_extension_.insert(std::make_pair(extension, rec));
		}
	}

	spl::shared_ptr<frame_producer> create_producer(
			const spl::shared_ptr<video_channel>& video_channel,
			const std::wstring& filename) const
	{
		auto found = find_record(filename);

		if (!found)
			return frame_producer::empty();

		return found->producer_factory(
				video_channel->frame_factory(),
				video_channel->video_format_desc(),
				filename);
	}

	spl::shared_ptr<cg_proxy> get_proxy(const spl::shared_ptr<frame_producer>& producer) const
	{
		auto producer_name = producer->name();

		boost::lock_guard<boost::mutex> lock(mutex_);

		for (auto& elem : records_by_extension_)
		{
			if (elem.second.name == producer_name)
				return elem.second.proxy_factory(producer);
		}

		return cg_proxy::empty();
	}

	spl::shared_ptr<cg_proxy> get_proxy(
			const spl::shared_ptr<video_channel>& video_channel,
			int render_layer) const
	{
		auto producer = spl::make_shared_ptr(video_channel->stage().foreground(render_layer).get());

		return get_proxy(producer);
	}

	spl::shared_ptr<cg_proxy> get_or_create_proxy(
			const spl::shared_ptr<video_channel>& video_channel,
			int render_layer,
			const std::wstring& filename) const
	{
		using namespace boost::filesystem;

		auto found = find_record(filename);

		if (!found)
			return cg_proxy::empty();

		auto producer = spl::make_shared_ptr(video_channel->stage().foreground(render_layer).get());
		auto current_producer_name = producer->name();
		bool create_new = current_producer_name != found->name || !found->reusable_producer_instance;

		if (create_new)
		{
			diagnostics::scoped_call_context save;
			diagnostics::call_context::for_thread().video_channel = video_channel->index();
			diagnostics::call_context::for_thread().layer = render_layer;

			producer = found->producer_factory(
					video_channel->frame_factory(),
					video_channel->video_format_desc(),
					filename);
			video_channel->stage().load(render_layer, producer);
			video_channel->stage().play(render_layer);
		}

		return found->proxy_factory(producer);
	}

	std::string read_meta_info(const std::wstring& filename) const
	{
		using namespace boost::filesystem;

		auto basepath = path(env::template_folder()) / path(filename);

		boost::lock_guard<boost::mutex> lock(mutex_);

		for (auto& rec : records_by_extension_)
		{
			auto p = path(basepath.wstring() + rec.first);
			auto found = find_case_insensitive(p.wstring());

			if (found)
				return rec.second.info_extractor(*found);
		}

		BOOST_THROW_EXCEPTION(caspar_exception() << msg_info(L"No meta info extractor for " + filename));
	}

	bool is_cg_extension(const std::wstring& extension) const
	{
		boost::lock_guard<boost::mutex> lock(mutex_);

		return records_by_extension_.find(extension) != records_by_extension_.end();
	}
private:
	boost::optional<record> find_record(const std::wstring& filename) const
	{
		using namespace boost::filesystem;

		auto basepath = path(env::template_folder()) / path(filename);

		boost::lock_guard<boost::mutex> lock(mutex_);

		for (auto& rec : records_by_extension_)
		{
			auto p = path(basepath.wstring() + rec.first);

			if (find_case_insensitive(p.wstring()))
				return rec.second;
		}

		return boost::none;
	}
};

cg_producer_registry::cg_producer_registry() : impl_(new impl) { }

void cg_producer_registry::register_cg_producer(
		std::wstring cg_producer_name,
		std::set<std::wstring> file_extensions,
		meta_info_extractor info_extractor,
		cg_proxy_factory proxy_factory,
		cg_producer_factory producer_factory,
		bool reusable_producer_instance)
{
	impl_->register_cg_producer(
			std::move(cg_producer_name),
			std::move(file_extensions),
			std::move(info_extractor),
			std::move(proxy_factory),
			std::move(producer_factory),
			reusable_producer_instance);
}

spl::shared_ptr<frame_producer> cg_producer_registry::create_producer(
		const spl::shared_ptr<video_channel>& video_channel,
		const std::wstring& filename) const
{
	return impl_->create_producer(video_channel, filename);
}

spl::shared_ptr<cg_proxy> cg_producer_registry::get_proxy(
		const spl::shared_ptr<frame_producer>& producer) const
{
	return impl_->get_proxy(producer);
}

spl::shared_ptr<cg_proxy> cg_producer_registry::get_proxy(
		const spl::shared_ptr<video_channel>& video_channel,
		int render_layer) const
{
	return impl_->get_proxy(video_channel, render_layer);
}

spl::shared_ptr<cg_proxy> cg_producer_registry::get_or_create_proxy(
		const spl::shared_ptr<video_channel>& video_channel,
		int render_layer,
		const std::wstring& filename) const
{
	return impl_->get_or_create_proxy(video_channel, render_layer, filename);
}

std::string cg_producer_registry::read_meta_info(const std::wstring& filename) const
{
	return impl_->read_meta_info(filename);
}

bool cg_producer_registry::is_cg_extension(const std::wstring& extension) const
{
	return impl_->is_cg_extension(extension);
}

}}
