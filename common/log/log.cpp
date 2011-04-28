/*
* copyright (c) 2010 Sveriges Television AB <info@casparcg.com>
*
*  This file is part of CasparCG.
*
*    CasparCG is free software: you can redistribute it and/or modify
*    it under the terms of the GNU General Public License as published by
*    the Free Software Foundation, either version 3 of the License, or
*    (at your option) any later version.
*
*    CasparCG is distributed in the hope that it will be useful,
*    but WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*    GNU General Public License for more details.

*    You should have received a copy of the GNU General Public License
*    along with CasparCG.  If not, see <http://www.gnu.org/licenses/>.
*
*/
#include "../stdafx.h"

#if defined(_MSC_VER)
#pragma warning (disable : 4100) // 'identifier' : unreferenced formal parameter
#pragma warning (disable : 4512) // 'class' : assignment operator could not be generated
#endif

#include "log.h"

#include "../exception/exceptions.h"

#include <ios>
#include <string>
#include <ostream>

#include <boost/shared_ptr.hpp>
#include <boost/make_shared.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

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

#include <boost/log/utility/init/common_attributes.hpp>
#include <boost/log/utility/empty_deleter.hpp>


namespace caspar{ namespace log{

using namespace boost;

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

	stream_sink->locked_backend()->set_formatter(
		boost::log::formatters::wstream
			<< L"[" << boost::log::formatters::attr<boost::log::attributes::current_thread_id::held_type >(traits_t::thread_id_attr_name())
			<< L"] [" << boost::log::formatters::attr<severity_level >(boost::log::sources::aux::severity_attribute_name<wchar_t>::get())
			<< L"] " << boost::log::formatters::message<wchar_t>()
	);

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
			boost::log::keywords::file_name = (folder + L"caspar_%Y-%m-%d_%H-%M-%S.%N.log"),
			boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
			boost::log::keywords::auto_flush = true
		);

		file_sink->locked_backend()->set_formatter(
			boost::log::formatters::wstream
				<< boost::log::formatters::attr<unsigned int>(traits_t::line_id_attr_name())
				<< L" [" << boost::log::formatters::date_time< posix_time::ptime >(traits_t::time_stamp_attr_name())
				<< L"] [" << boost::log::formatters::attr<boost::log::attributes::current_thread_id::held_type >(traits_t::thread_id_attr_name())
				<< L"] [" << boost::log::formatters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get())
				<< L"] " << boost::log::formatters::message<wchar_t>()
		);

		file_sink->set_filter(boost::log::filters::attr<severity_level>(boost::log::sources::aux::severity_attribute_name<wchar_t>::get()) >= info);
		
		boost::log::wcore::get()->add_sink(file_sink);

		CASPAR_LOG(info) << L"Logging [info] or higher severity to " << folder << std::endl << std::endl;
	}
	catch(...)
	{
		std::wcerr << L"Failed to Setup File Logging Sink" << std::endl << std::endl;
	}
}

}}