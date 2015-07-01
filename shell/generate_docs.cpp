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

#include "included_modules.h"

#include <common/env.h>
#include <common/log.h>

#include <core/producer/media_info/in_memory_media_info_repository.h>
#include <core/help/help_repository.h>
#include <core/help/help_sink.h>
#include <core/help/util.h>

#include <protocol/amcp/amcp_command_repository.h>
#include <protocol/amcp/AMCPCommandsImpl.h>

#include <boost/filesystem/fstream.hpp>

#include <iostream>
#include <sstream>

using namespace caspar;

static const int WIDTH = 150;

class mediawiki_paragraph_builder : public core::paragraph_builder
{
	std::wostringstream	out_;
	std::wostream&		commit_to_;
public:
	mediawiki_paragraph_builder(std::wostream& out)
		: commit_to_(out)
	{
	}

	~mediawiki_paragraph_builder()
	{
		commit_to_ << core::wordwrap(out_.str(), WIDTH) << std::endl;
	}

	spl::shared_ptr<paragraph_builder> text(std::wstring text) override
	{
		out_ << std::move(text);
		return shared_from_this();
	};

	spl::shared_ptr<paragraph_builder> code(std::wstring text) override
	{
		out_ << L"<code>" << std::move(text) << L"</code>";
		return shared_from_this();
	};

	spl::shared_ptr<paragraph_builder> see(std::wstring item) override
	{
		out_ << L"[[#" << item << L"|" << item << L"]]";
		return shared_from_this();
	};

	spl::shared_ptr<paragraph_builder> url(std::wstring url, std::wstring name) override
	{
		out_ << L"[" << std::move(url) << L" " << std::move(name) << L"]";
		return shared_from_this();
	};
};

class mediawiki_definition_list_builder : public core::definition_list_builder
{
	std::wostream& out_;
public:
	mediawiki_definition_list_builder(std::wostream& out)
		: out_(out)
	{
	}

	~mediawiki_definition_list_builder()
	{
		out_ << L"\n" << std::endl;
	}

	spl::shared_ptr<definition_list_builder> item(std::wstring term, std::wstring description) override
	{
		out_ << L"; <code>" << term << L"</code>\n";
		out_ << L": " << description << L"\n";

		return shared_from_this();
	};
};

class mediawiki_help_sink : public core::help_sink
{
	std::wostream& out_;
public:
	mediawiki_help_sink(std::wostream& out)
		: out_(out)
	{
	}

	void start_section(std::wstring title)
	{
		out_ << L"=" << title << L"=\n" << std::endl;
	}

	void syntax(const std::wstring& syntax) override
	{
		out_ << L"Syntax:\n";
		out_ << core::indent(core::wordwrap(syntax, WIDTH - 1), L" ") << std::endl;
	}

	spl::shared_ptr<core::paragraph_builder> para() override
	{
		return spl::make_shared<mediawiki_paragraph_builder>(out_);
	}

	spl::shared_ptr<core::definition_list_builder> definitions() override
	{
		return spl::make_shared<mediawiki_definition_list_builder>(out_);
	}

	void example(const std::wstring& code, const std::wstring& caption) override
	{
		out_ << core::indent(core::wordwrap(code, WIDTH - 1), L" ");

		if (!caption.empty())
			out_ << core::wordwrap(L"..." + caption, WIDTH - 1);

		out_ << std::endl;
	}
private:
	void begin_item(const std::wstring& name) override 
	{
		out_ << L"==" << name << L"==\n" << std::endl;
	}
};

void generate_amcp_commands_help(const core::help_repository& help_repo)
{
	boost::filesystem::wofstream file(L"amcp_commands_help.wiki");
	mediawiki_help_sink sink(file);

	auto print_section = [&](std::wstring title)
	{
		sink.start_section(title);
		help_repo.help({ L"AMCP", title }, sink);
	};

	print_section(L"Basic Commands");
	print_section(L"Data Commands");
	print_section(L"Template Commands");
	print_section(L"Mixer Commands");
	print_section(L"Thumbnail Commands");
	print_section(L"Query Commands");
	file.flush();
}

int main(int argc, char** argv)
{
	//env::configure(L"casparcg.config");
	//log::set_log_level(L"info");

	spl::shared_ptr<core::system_info_provider_repository> system_info_provider_repo;
	spl::shared_ptr<core::cg_producer_registry> cg_registry;
	auto media_info_repo = core::create_in_memory_media_info_repository();
	spl::shared_ptr<core::help_repository> help_repo;
	spl::shared_ptr<core::frame_producer_registry> producer_registry;
	std::promise<bool> shutdown_server_now;
	protocol::amcp::amcp_command_repository repo(
			{ },
			nullptr,
			media_info_repo,
			system_info_provider_repo,
			cg_registry,
			help_repo,
			producer_registry,
			shutdown_server_now);

	protocol::amcp::register_commands(repo);

	core::module_dependencies dependencies(system_info_provider_repo, cg_registry, media_info_repo, producer_registry);
	initialize_modules(dependencies);

	generate_amcp_commands_help(*help_repo);

	uninitialize_modules();
	
	return 0;
}
