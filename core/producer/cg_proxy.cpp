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
#include "variable.h"

#include "frame_producer.h"
#include "stage.h"
#include "../video_channel.h"
#include "../diagnostics/call_context.h"
#include "../module_dependencies.h"
#include "../frame/draw_frame.h"

#include <common/env.h>
#include <common/os/filesystem.h>
#include <common/future.h>

#include <boost/filesystem.hpp>
#include <boost/algorithm/string/predicate.hpp>
#include <boost/optional.hpp>
#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>

#include <future>
#include <map>
#include <sstream>

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

	mutable std::mutex			    mutex_;
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
		std::lock_guard<std::mutex> lock(mutex_);

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
			const frame_producer_dependencies& dependencies,
			const std::wstring& filename) const
	{
		auto found = find_record(filename);

		if (!found)
			return frame_producer::empty();

		return found->producer_factory(dependencies, filename);
	}

	spl::shared_ptr<cg_proxy> get_proxy(const spl::shared_ptr<frame_producer>& producer) const
	{
		auto producer_name = producer->name();

		std::lock_guard<std::mutex> lock(mutex_);

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
			const frame_producer_dependencies& dependencies,
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

			producer = found->producer_factory(dependencies, filename);
			video_channel->stage().load(render_layer, producer);
			video_channel->stage().play(render_layer);
		}

		return found->proxy_factory(producer);
	}

	std::string read_meta_info(const std::wstring& filename) const
	{
		using namespace boost::filesystem;

		auto basepath = path(env::template_folder()) / path(filename);

		std::lock_guard<std::mutex> lock(mutex_);

		for (auto& rec : records_by_extension_)
		{
			auto p = path(basepath.wstring() + rec.first);
			auto found = find_case_insensitive(p.wstring());

			if (found)
				return rec.second.info_extractor(*found);
		}

		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"No meta info extractor for " + filename));
	}

	bool is_cg_extension(const std::wstring& extension) const
	{
		std::lock_guard<std::mutex> lock(mutex_);

		return records_by_extension_.find(extension) != records_by_extension_.end();
	}

	std::wstring get_cg_producer_name(const std::wstring& filename) const
	{
		auto record = find_record(filename);

		if (!record)
			CASPAR_THROW_EXCEPTION(caspar_exception() << msg_info(filename + L" is not a cg template."));

		return record->name;
	}
private:
	boost::optional<record> find_record(const std::wstring& filename) const
	{
		using namespace boost::filesystem;

		auto basepath = path(env::template_folder()) / path(filename);

		std::lock_guard<std::mutex> lock(mutex_);

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
		const frame_producer_dependencies& dependencies,
		const std::wstring& filename) const
{
	return impl_->create_producer(dependencies, filename);
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
		const frame_producer_dependencies& dependencies,
		int render_layer,
		const std::wstring& filename) const
{
	return impl_->get_or_create_proxy(video_channel, dependencies, render_layer, filename);
}

std::string cg_producer_registry::read_meta_info(const std::wstring& filename) const
{
	return impl_->read_meta_info(filename);
}

bool cg_producer_registry::is_cg_extension(const std::wstring& extension) const
{
	return impl_->is_cg_extension(extension);
}

std::wstring cg_producer_registry::get_cg_producer_name(const std::wstring& filename) const
{
	return impl_->get_cg_producer_name(filename);
}

class cg_proxy_as_producer : public frame_producer
{
	spl::shared_ptr<frame_producer>							producer_;
	spl::shared_ptr<cg_proxy>								proxy_;
	std::wstring											template_name_;
	bool													producer_has_its_own_variables_defined_;

	std::map<std::wstring, std::shared_ptr<core::variable>>	variables_;
	std::vector<std::wstring>								variable_names_;
	core::binding<std::wstring>								template_data_xml_						{ [=] { return generate_template_data_xml(); } };
	std::shared_ptr<void>									template_data_change_subscription_;
	bool													is_playing_								= false;
public:
	cg_proxy_as_producer(
			spl::shared_ptr<frame_producer> producer,
			spl::shared_ptr<cg_proxy> proxy,
			const std::wstring& template_name,
			const std::vector<std::wstring>& parameter_specification)
		: producer_(std::move(producer))
		, proxy_(std::move(proxy))
		, template_name_(std::move(template_name))
		, producer_has_its_own_variables_defined_(!producer_->get_variables().empty())
	{
		if (parameter_specification.size() % 2 != 0)
			CASPAR_THROW_EXCEPTION(user_error() << msg_info("Parameter specification must be a sequence of type and parameter name pairs"));

		if (producer_has_its_own_variables_defined_ && !parameter_specification.empty())
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(L"Producer " + producer_->name() + L" does not need help with available template parameters"));

		for (int i = 0; i < parameter_specification.size(); i += 2)
		{
			auto& type				= parameter_specification.at(i);
			auto& parameter_name	= parameter_specification.at(i + 1);

			if (type == L"string")
				create_parameter<std::wstring>(parameter_name);
			else if (type == L"number")
				create_parameter<double>(parameter_name);
			else
				CASPAR_THROW_EXCEPTION(user_error() << msg_info("The type in a parameter specification must be either string or number"));
		}

		template_data_change_subscription_ = template_data_xml_.on_change([=]
		{
			if (is_playing_)
				proxy_->update(0, template_data_xml_.get());
		});
	}

	// frame_producer

	std::future<std::wstring> call(const std::vector<std::wstring>& params) override
	{
		auto& command = params.at(0);

		if (command == L"play()")
		{
			proxy_->add(0, template_name_, true, L"", template_data_xml_.get());
			is_playing_ = true;
		}
		else if (command == L"stop()")
			proxy_->stop(0, 0);
		else if (command == L"next()")
			proxy_->next(0);
		else if (command == L"invoke()")
			proxy_->invoke(0, params.at(1));
		else
			return producer_->call(params);

		return make_ready_future<std::wstring>(L"");
	}

	variable& get_variable(const std::wstring& name) override
	{
		if (producer_has_its_own_variables_defined_)
			return producer_->get_variable(name);

		auto found = variables_.find(name);

		if (found == variables_.end())
			CASPAR_THROW_EXCEPTION(user_error() << msg_info(name + L" not found in scene"));

		return *found->second;
	}

	const std::vector<std::wstring>& get_variables() const override
	{
		return producer_has_its_own_variables_defined_ ? producer_->get_variables() : variable_names_;
	}

	draw_frame							receive() override															{ return producer_->receive(); }
	std::wstring						print() const override														{ return producer_->print(); }
	void								paused(bool value) override													{ producer_->paused(value); }
	std::wstring						name() const override														{ return producer_->name(); }
	uint32_t							frame_number() const override												{ return producer_->frame_number(); }
	boost::property_tree::wptree 		info() const override														{ return producer_->info(); }
	void								leading_producer(const spl::shared_ptr<frame_producer>& producer) override	{ return producer_->leading_producer(producer); }
	uint32_t							nb_frames() const override													{ return producer_->nb_frames(); }
	draw_frame							last_frame()																{ return producer_->last_frame(); }
	monitor::subject&					monitor_output() override													{ return producer_->monitor_output(); }
	bool								collides(double x, double y) const override									{ return producer_->collides(x, y); }
	void								on_interaction(const interaction_event::ptr& event)	override				{ return producer_->on_interaction(event); }
	constraints&						pixel_constraints() override												{ return producer_->pixel_constraints(); }
private:
	std::wstring generate_template_data_xml() const
	{
		boost::property_tree::wptree document;
		boost::property_tree::wptree template_data;

		for (auto& parameter_name : variable_names_)
		{
			boost::property_tree::wptree component_data;

			component_data.add(L"<xmlattr>.id", parameter_name);
			component_data.add(L"data.<xmlattr>.id", L"text");
			component_data.add(L"data.<xmlattr>.value", variables_.at(parameter_name)->to_string());

			template_data.add_child(L"componentData", component_data);
		}

		document.add_child(L"templateData", template_data);

		std::wstringstream xml;
		static boost::property_tree::xml_writer_settings<std::wstring> NO_INDENTATION_SETTINGS(' ', 0);
		boost::property_tree::xml_parser::write_xml(xml, document, NO_INDENTATION_SETTINGS);
		// Necessary hack
		static std::wstring PROCESSING_INSTRUCTION_TO_REMOVE = L"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n";
		return xml.str().substr(PROCESSING_INSTRUCTION_TO_REMOVE.length());
	}

	template<typename T>
	void create_parameter(const std::wstring& name)
	{
		auto var = spl::make_shared<core::variable_impl<T>>();

		template_data_xml_.depend_on(var->value());
		variables_.insert(std::make_pair(name, var));
		variable_names_.push_back(name);
	}
};

spl::shared_ptr<frame_producer> create_cg_proxy_as_producer(const core::frame_producer_dependencies& dependencies, const std::vector<std::wstring>& params)
{
	if (!boost::iequals(params.at(0), L"[CG]") || params.size() < 2)
		return frame_producer::empty();

	auto template_name	= params.at(1);
	auto producer		= dependencies.cg_registry->create_producer(dependencies, template_name);
	auto proxy			= dependencies.cg_registry->get_proxy(producer);
	auto params2		= params;

	if (proxy == cg_proxy::empty())
		CASPAR_THROW_EXCEPTION(file_not_found() << msg_info(L"No template with name " + template_name + L" found"));

	// Remove "[CG]" and template_name to only leave the parameter specification part
	params2.erase(params2.begin(), params2.begin() + 2);

	return spl::make_shared<cg_proxy_as_producer>(std::move(producer), std::move(proxy), template_name, params2);
}

void init_cg_proxy_as_producer(core::module_dependencies dependencies)
{
}

}}
