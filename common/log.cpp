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
* Author: Robert Nagy, ronag89@gmail.com
*/

#include "stdafx.h"

#include "log.h"

#include "except.h"
#include "utf.h"

#include <ios>
#include <iomanip>
#include <string>
#include <ostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>

#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/core/null_deleter.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <tbb/atomic.h>

namespace caspar { namespace log {

using namespace boost;

template<typename Stream>
void append_timestamp(Stream& stream)
{
	auto timestamp = boost::posix_time::microsec_clock::local_time();
	auto date = timestamp.date();
	auto time = timestamp.time_of_day();
	auto milliseconds = time.fractional_seconds() / 1000; // microseconds to milliseconds

	std::wstringstream buffer;

	buffer
		<< std::setfill(L'0')
		<< L"["
		<< std::setw(4) << date.year() << L"-" << std::setw(2) << date.month().as_number() << "-" << std::setw(2) << date.day().as_number()
		<< L" "
		<< std::setw(2) << time.hours() << L":" << std::setw(2) << time.minutes() << L":" << std::setw(2) << time.seconds()
		<< L"."
		<< std::setw(3) << milliseconds
		<< L"] ";

	stream << buffer.str();
}

class column_writer
{
	tbb::atomic<int> column_width_;
public:
	column_writer(int initial_width = 0)
	{
		column_width_ = initial_width;
	}

	template<typename Stream, typename Val>
	void write(Stream& out, const Val& value)
	{
		std::string to_string = boost::lexical_cast<std::string>(value);
		int length = static_cast<int>(to_string.size());
		int read_width;

		while ((read_width = column_width_) < length && column_width_.compare_and_swap(length, read_width) != read_width);
		read_width = column_width_;

		out << L"[" << to_string << L"] ";

		for (int n = 0; n < read_width - length; ++n)
			out << L" ";
	}
};

template<typename Stream>
void my_formatter(bool print_all_characters, const boost::log::record_view& rec, Stream& strm)
{
	static column_writer thread_id_column;
	static column_writer severity_column(7);
	namespace expr = boost::log::expressions;

	append_timestamp(strm);

	thread_id_column.write(strm, boost::log::extract<boost::log::attributes::current_thread_id::value_type>("ThreadID", rec).get().native_id());
	severity_column.write(strm, boost::log::extract<boost::log::trivial::severity_level>("Severity", rec));

	if (print_all_characters)
	{
		strm << rec[expr::message];
	}
	else
	{
		strm << replace_nonprintable_copy(rec[expr::message].get<std::wstring>(), L'?');
	}
}

namespace internal{
	
void init()
{	
	boost::log::add_common_attributes();
	typedef boost::log::sinks::asynchronous_sink<boost::log::sinks::wtext_ostream_backend> stream_sink_type;

	auto stream_backend = boost::make_shared<boost::log::sinks::wtext_ostream_backend>();
	stream_backend->add_stream(boost::shared_ptr<std::wostream>(&std::wcout, boost::null_deleter()));
	stream_backend->auto_flush(true);

	auto stream_sink = boost::make_shared<stream_sink_type>(stream_backend);

	bool print_all_characters = false;
	stream_sink->set_formatter(boost::bind(&my_formatter<boost::log::wformatting_ostream>, print_all_characters, _1, _2));

	boost::log::core::get()->add_sink(stream_sink);
}

}

void add_file_sink(const std::wstring& folder)
{
	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> file_sink_type;

	try
	{
		if (!boost::filesystem::is_directory(folder))
			CASPAR_THROW_EXCEPTION(directory_not_found());

		auto file_sink = boost::make_shared<file_sink_type>(
			boost::log::keywords::file_name = (folder + L"caspar_%Y-%m-%d.log"),
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
			boost::log::keywords::auto_flush = true,
			boost::log::keywords::open_mode = std::ios::app
			);

		bool print_all_characters = true;

		file_sink->set_formatter(boost::bind(&my_formatter<boost::log::formatting_ostream>, print_all_characters, _1, _2));
		boost::log::core::get()->add_sink(file_sink);
	}
	catch (...)
	{
		std::wcerr << L"Failed to Setup File Logging Sink" << std::endl << std::endl;
	}
}

void set_log_level(const std::wstring& lvl)
{
	if (boost::iequals(lvl, L"trace"))
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
	else if (boost::iequals(lvl, L"debug"))
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::debug);
	else if (boost::iequals(lvl, L"info"))
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::info);
	else if (boost::iequals(lvl, L"warning"))
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::warning);
	else if (boost::iequals(lvl, L"error"))
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::error);
	else if (boost::iequals(lvl, L"fatal"))
		boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::fatal);
}

void print_child(
		boost::log::trivial::severity_level level,
		const std::wstring& indent,
		const std::wstring& elem,
		const boost::property_tree::wptree& tree)
{
	auto& data = tree.data();

	if (!data.empty())
		BOOST_LOG_STREAM_WITH_PARAMS(log::logger::get(), (boost::log::keywords::severity = level)) << indent << elem << L" " << replace_nonprintable_copy(data, L'?');
	else if (tree.size() == 0)
		BOOST_LOG_STREAM_WITH_PARAMS(log::logger::get(), (boost::log::keywords::severity = level)) << indent << elem;

	for (auto& child : tree)
		print_child(level, indent + (elem.empty() ? L"" : elem + L"."), child.first, child.second);
}

}}
