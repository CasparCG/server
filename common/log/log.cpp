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

#include "../stdafx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4100) // 'identifier' : unreferenced formal parameter
#pragma warning (disable : 4512) // 'class' : assignment operator could not be generated
#endif

#include "log.h"

#include "../exception/exceptions.h"
#include "../utility/string.h"
#include <ios>
#include <iomanip>
#include <string>
#include <ostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/algorithm/string.hpp>

#include <boost/log/core/core.hpp>

#include <boost/log/formatters/stream.hpp>
#include <boost/log/formatters/attr.hpp>
#include <boost/log/formatters/date_time.hpp>
#include <boost/log/formatters/message.hpp>

#include <boost/log/filters/attr.hpp>

#include <boost/log/sinks/text_file_backend.hpp>

#include <boost/log/detail/universal_path.hpp>

#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/utility/attribute_value_extractor.hpp>

#include <boost/log/utility/init/common_attributes.hpp>
#include <boost/log/utility/empty_deleter.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/bind.hpp>

namespace caspar { namespace log {

using namespace boost;

void append_timestamp(std::wostream& stream)
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

void my_formatter(bool print_all_characters, std::wostream& strm, boost::log::basic_record<wchar_t> const& rec)
{
    namespace lambda = boost::lambda;
	
	#pragma warning(disable : 4996)

	append_timestamp(strm);
		
    boost::log::attributes::current_thread_id::held_type thread_id;
    if(boost::log::extract<boost::log::attributes::current_thread_id::held_type>(L"ThreadID", rec.attribute_values(), lambda::var(thread_id) = lambda::_1))
        strm << L"[" << thread_id << L"] ";


    severity_level severity;
    if(boost::log::extract<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get(), rec.attribute_values(), lambda::var(severity) = lambda::_1))
	{
		std::stringstream ss;
		ss << severity;
        strm << L"[" << severity << L"] ";
		for(int n = 0; n < 7-static_cast<int>(ss.str().size()); ++n)
			strm << L" ";
	}

	if (print_all_characters)
	{
		strm << rec.message();
	}
	else
	{
	    strm << replace_nonprintable_copy(rec.message(), L'?');
	}
}

namespace internal{
	
void init()
{	
	boost::log::add_common_attributes<wchar_t>();
	typedef boost::log::aux::add_common_attributes_constants<wchar_t> traits_t;

	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::wtext_file_backend> file_sink_type;

	typedef boost::log::sinks::asynchronous_sink<boost::log::sinks::wtext_ostream_backend> stream_sink_type;

	auto stream_backend = boost::make_shared<boost::log::sinks::wtext_ostream_backend>();
	stream_backend->add_stream(boost::shared_ptr<std::wostream>(&std::wcout, boost::log::empty_deleter()));
	stream_backend->auto_flush(true);

	auto stream_sink = boost::make_shared<stream_sink_type>(stream_backend);
	
//#ifdef NDEBUG
//	stream_sink->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= debug);
//#else
//	stream_sink->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= debug);
//#endif

	stream_sink->locked_backend()->set_formatter(boost::bind(my_formatter, false, _1, _2));

	boost::log::wcore::get()->add_sink(stream_sink);
}

}

void add_file_sink(const std::wstring& folder)
{	
	boost::log::add_common_attributes<wchar_t>();
	typedef boost::log::aux::add_common_attributes_constants<wchar_t> traits_t;

	typedef boost::log::sinks::synchronous_sink<boost::log::sinks::wtext_file_backend> file_sink_type;

	try
	{
		if(!boost::filesystem::is_directory(folder))
			BOOST_THROW_EXCEPTION(directory_not_found());

		auto file_sink = boost::make_shared<file_sink_type>(
			boost::log::keywords::file_name = (folder + L"caspar_%Y-%m-%d.log"),
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
			boost::log::keywords::auto_flush = true,
			boost::log::keywords::open_mode = std::ios::app
		);

		bool print_all_characters = true;

		file_sink->locked_backend()->set_formatter(boost::bind(my_formatter, print_all_characters, _1, _2));

//#ifdef NDEBUG
//		file_sink->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= debug);
//#else
//		file_sink->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= debug);
//#endif
		boost::log::wcore::get()->add_sink(file_sink);
	}
	catch(...)
	{
		std::wcerr << L"Failed to Setup File Logging Sink" << std::endl << std::endl;
	}
}

void set_log_level(const std::wstring& lvl)
{	
	if(boost::iequals(lvl, L"trace"))
		boost::log::wcore::get()->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= trace);
	else if(boost::iequals(lvl, L"debug"))
		boost::log::wcore::get()->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= debug);
	else if(boost::iequals(lvl, L"info"))
		boost::log::wcore::get()->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= info);
	else if(boost::iequals(lvl, L"warning"))
		boost::log::wcore::get()->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= warning);
	else if(boost::iequals(lvl, L"error"))
		boost::log::wcore::get()->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= error);
	else if(boost::iequals(lvl, L"fatal"))
		boost::log::wcore::get()->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= fatal);
}

}}