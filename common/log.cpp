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
#include "log.h"

#include "except.h"
#include "utf.h"

#include <iomanip>
#include <ios>
#include <ostream>
#include <string>

#include <boost/algorithm/string.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/filesystem/convenience.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/trivial.hpp>

#include <boost/log/attributes/attribute_value.hpp>
#include <boost/log/attributes/function.hpp>
#include <boost/log/core/record.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>

#include <boost/core/null_deleter.hpp>
#include <boost/lambda/lambda.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <atomic>
#include <mutex>

namespace caspar { namespace log {

using namespace boost;

template <typename Stream>
void append_timestamp(Stream& stream, boost::posix_time::ptime timestamp)
{
    auto date         = timestamp.date();
    auto time         = timestamp.time_of_day();
    auto milliseconds = time.fractional_seconds() / 1000; // microseconds to milliseconds

    std::wstringstream buffer;

    buffer << std::setfill(L'0') << L"[" << std::setw(4) << date.year() << L"-" << std::setw(2)
           << date.month().as_number() << "-" << std::setw(2) << date.day().as_number() << L" " << std::setw(2)
           << time.hours() << L":" << std::setw(2) << time.minutes() << L":" << std::setw(2) << time.seconds() << L"."
           << std::setw(3) << milliseconds << L"] ";

    stream << buffer.str();
}

class column_writer
{
    std::atomic<int> column_width_;

  public:
    column_writer(int initial_width = 0) { column_width_ = initial_width; }

    template <typename Stream, typename Val>
    void write(Stream& out, const Val& value)
    {
        std::wstring to_string = boost::lexical_cast<std::wstring>(value);
        int          length    = static_cast<int>(to_string.size());
        int          read_width;

        while ((read_width = column_width_) < length &&
               column_width_.compare_exchange_strong(length, read_width) != read_width)
            ;
        read_width = column_width_;

        out << L"[" << to_string << L"] ";

        for (int n = 0; n < read_width - length; ++n)
            out << L" ";
    }
};

template <typename Stream>
void my_formatter(bool print_all_characters, const boost::log::record_view& rec, Stream& strm)
{
    static column_writer thread_id_column;
    static column_writer severity_column(7);
    namespace expr = boost::log::expressions;

    std::wstringstream pre_message_stream;
    append_timestamp(pre_message_stream, boost::log::extract<boost::posix_time::ptime>("TimestampMillis", rec).get());
    thread_id_column.write(pre_message_stream, boost::log::extract<std::int64_t>("NativeThreadId", rec));
    severity_column.write(pre_message_stream,
                          boost::log::extract<boost::log::trivial::severity_level>("Severity", rec));

    auto pre_message = pre_message_stream.str();

    strm << pre_message;

    auto line_break_replacement = L"\n" + pre_message;

    if (print_all_characters) {
        strm << boost::replace_all_copy(rec[expr::message].get<std::wstring>(), "\n", line_break_replacement);
    } else {
        strm << boost::replace_all_copy(
            replace_nonprintable_copy(rec[expr::message].get<std::wstring>(), L'?'), L"\n", line_break_replacement);
    }
}

namespace internal {

void init()
{
    boost::log::add_common_attributes();
    boost::log::core::get()->add_global_attribute("NativeThreadId",
                                                  boost::log::attributes::make_function(&std::this_thread::get_id));
    boost::log::core::get()->add_global_attribute("TimestampMillis", boost::log::attributes::make_function([] {
                                                      return boost::posix_time::microsec_clock::local_time();
                                                  }));
    typedef boost::log::sinks::asynchronous_sink<boost::log::sinks::wtext_ostream_backend> stream_sink_type;

    auto stream_backend = boost::make_shared<boost::log::sinks::wtext_ostream_backend>();
    stream_backend->add_stream(boost::shared_ptr<std::wostream>(&std::wcout, boost::null_deleter()));
    stream_backend->auto_flush(true);

    auto stream_sink = boost::make_shared<stream_sink_type>(stream_backend);
    // Never log calltrace to console. The terminal is too slow, so the log queue will build up faster than consumed.
    stream_sink->set_filter(category != log_category::calltrace);

    // bool print_all_characters = false;
    // stream_sink->set_formatter(boost::bind(&my_formatter<boost::log::wformatting_ostream>, print_all_characters, _1,
    // _2));

    boost::log::core::get()->add_sink(stream_sink);
}

std::string current_exception_diagnostic_information()
{
    auto e = boost::current_exception_cast<const char*>();

    if (e)
        return std::string("[char *] = ") + *e + "\n";
    else
        return boost::current_exception_diagnostic_information();
}

} // namespace internal

void add_file_sink(const std::wstring& file, const boost::log::filter& filter)
{
    typedef boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend> file_sink_type;

    try {
        if (!boost::filesystem::is_directory(boost::filesystem::path(file).parent_path()))
            CASPAR_THROW_EXCEPTION(directory_not_found());

        auto file_sink = boost::make_shared<file_sink_type>(
            boost::log::keywords::file_name           = (file + L"_%Y-%m-%d.log"),
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
            boost::log::keywords::auto_flush          = true,
            boost::log::keywords::open_mode           = std::ios::app);

        // bool print_all_characters = true;

        // file_sink->set_formatter(boost::bind(&my_formatter<boost::log::formatting_ostream>, print_all_characters, _1,
        // _2));
        file_sink->set_filter(filter);
        boost::log::core::get()->add_sink(file_sink);
    } catch (...) {
        std::wcerr << L"Failed to Setup File Logging Sink" << std::endl << std::endl;
    }
}

std::shared_ptr<void> add_preformatted_line_sink(std::function<void(std::string line)> formatted_line_sink)
{
    class sink_backend : public boost::log::sinks::basic_formatted_sink_backend<char>
    {
        std::function<void(std::string line)> formatted_line_sink_;

      public:
        //		sink_backend(std::function<void(std::string line)> formatted_line_sink)
        //			: formatted_line_sink_(std::move(formatted_line_sink))
        //		{
        //		}

        void consume(const boost::log::record_view& rec, const std::string& formatted_message)
        {
            try {
                formatted_line_sink_(formatted_message);
            } catch (...) {
                std::cerr << "[sink_backend] Error while consuming formatted message: " << formatted_message
                          << std::endl
                          << std::endl;
            }
        }
    };

    //	typedef boost::log::sinks::synchronous_sink<sink_backend> sink_type;

    //	auto sink = boost::make_shared<sink_type>(std::move(formatted_line_sink));
    //	bool print_all_characters = true;

    //	sink->set_formatter(boost::bind(&my_formatter<boost::log::formatting_ostream>, print_all_characters, _1, _2));
    //	sink->set_filter(category != log_category::calltrace);

    //	boost::log::core::get()->add_sink(sink);

    return std::shared_ptr<void>(nullptr, [=](void*) {
        //		boost::log::core::get()->remove_sink(sink);
    });
}

std::mutex& get_filter_mutex()
{
    static std::mutex instance;

    return instance;
}

boost::log::trivial::severity_level& get_level()
{
    static boost::log::trivial::severity_level instance;

    return instance;
}

log_category& get_disabled_categories()
{
    static log_category instance = log_category::calltrace;

    return instance;
}

void set_log_filter()
{
    auto severity_filter     = boost::log::trivial::severity >= get_level();
    auto disabled_categories = get_disabled_categories();

    boost::log::core::get()->set_filter([=](const boost::log::attribute_value_set& attributes) {
        return severity_filter(attributes) &&
               static_cast<int>(disabled_categories & attributes["Channel"].extract<log_category>().get()) == 0;
    });
}

void set_log_level(const std::wstring& lvl)
{
    std::lock_guard<std::mutex> lock(get_filter_mutex());

    if (boost::iequals(lvl, L"trace"))
        get_level() = boost::log::trivial::trace;
    else if (boost::iequals(lvl, L"debug"))
        get_level() = boost::log::trivial::debug;
    else if (boost::iequals(lvl, L"info"))
        get_level() = boost::log::trivial::info;
    else if (boost::iequals(lvl, L"warning"))
        get_level() = boost::log::trivial::warning;
    else if (boost::iequals(lvl, L"error"))
        get_level() = boost::log::trivial::error;
    else if (boost::iequals(lvl, L"fatal"))
        get_level() = boost::log::trivial::fatal;

    set_log_filter();
}

void set_log_category(const std::wstring& cat, bool enabled)
{
    log_category category_to_set;

    if (boost::iequals(cat, L"calltrace"))
        category_to_set = log_category::calltrace;
    else if (boost::iequals(cat, L"communication"))
        category_to_set = log_category::communication;
    else
        return; // Ignore

    std::lock_guard<std::mutex> lock(get_filter_mutex());
    auto&                       disabled_categories = get_disabled_categories();

    if (enabled)
        disabled_categories &= ~category_to_set;
    else
        disabled_categories |= category_to_set;

    set_log_filter();
}

}} // namespace caspar::log
