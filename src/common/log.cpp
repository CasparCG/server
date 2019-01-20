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

#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/locale.hpp>
#include <boost/log/attributes/function.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/logger.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/smart_ptr/make_shared_object.hpp>
#include <boost/smart_ptr/shared_ptr.hpp>

#include <atomic>
#include <iostream>

namespace logging  = boost::log;
namespace src      = boost::log::sources;
namespace sinks    = boost::log::sinks;
namespace keywords = boost::log::keywords;

namespace caspar { namespace log {

std::string current_exception_diagnostic_information()
{
    {
        auto e = boost::current_exception_cast<const char*>();
        if (e != nullptr) {
            return std::string("[char *] = ") + *e + "\n";
        }
    }
    {
        auto e = boost::current_exception_cast<const wchar_t*>();
        if (e != nullptr) {
            return std::string("[char *] = ") + u8(*e) + "\n";
        }
    }
    {
        auto e = boost::current_exception_cast<std::string>();
        if (e != nullptr) {
            return std::string("[char *] = ") + *e + "\n";
        }
    }
    {
        auto e = boost::current_exception_cast<std::wstring>();
        if (e != nullptr) {
            return std::string("[char *] = ") + u8(*e) + "\n";
        }
    }

    return boost::current_exception_diagnostic_information(true);
}

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
    column_writer(int initial_width = 0)
        : column_width_(initial_width)
    {
    }

    template <typename Stream, typename Val>
    void write(Stream& out, const Val& value)
    {
        std::wstring to_string = boost::lexical_cast<std::wstring>(value);
        int          length    = static_cast<int>(to_string.size());
        int          read_width;

        while (true) {
            read_width = column_width_;
            if (read_width >= length || column_width_.compare_exchange_strong(length, read_width)) {
                break;
            }
        }

        read_width = column_width_;

        out << L"[" << to_string << L"] ";

        for (int n = 0; n < read_width - length; ++n) {
            out << L" ";
        }
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
    // thread_id_column.write(pre_message_stream, boost::log::extract<std::int64_t>("NativeThreadId", rec));
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

void add_file_sink(const std::wstring& file)
{
    using file_sink_type = boost::log::sinks::synchronous_sink<boost::log::sinks::text_file_backend>;

    try {
        if (!boost::filesystem::is_directory(boost::filesystem::path(file).parent_path())) {
            CASPAR_THROW_EXCEPTION(directory_not_found());
        }

        auto file_sink = boost::make_shared<file_sink_type>(
            boost::log::keywords::file_name           = file + L"_%Y-%m-%d.log",
            boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
            boost::log::keywords::auto_flush          = true,
            boost::log::keywords::open_mode           = std::ios::app);

        file_sink->set_formatter(boost::bind(&my_formatter<boost::log::formatting_ostream>, true, _1, _2));

        boost::log::core::get()->add_sink(file_sink);
    } catch (...) {
        std::wcerr << L"Failed to Setup File Logging Sink" << std::endl << std::endl;
    }
}

void add_cout_sink()
{
    boost::log::add_common_attributes();
    // boost::log::core::get()->add_global_attribute("NativeThreadId",
    // boost::log::attributes::make_function(&std::this_thread::get_id));
    boost::log::core::get()->add_global_attribute("TimestampMillis", boost::log::attributes::make_function([] {
                                                      return boost::posix_time::microsec_clock::local_time();
                                                  }));

    using stream_sink_type = sinks::asynchronous_sink<sinks::wtext_ostream_backend>;

    auto stream_backend = boost::make_shared<boost::log::sinks::wtext_ostream_backend>();
    stream_backend->add_stream(boost::shared_ptr<std::wostream>(&std::wcout, boost::null_deleter()));
    stream_backend->auto_flush(true);

    auto stream_sink = boost::make_shared<stream_sink_type>(stream_backend);

    stream_sink->set_formatter(boost::bind(&my_formatter<boost::log::wformatting_ostream>, false, _1, _2));

    logging::core::get()->add_sink(stream_sink);
}

std::wstring current_log_level;

bool set_log_level(const std::wstring& lvl)
{
    if (boost::iequals(lvl, L"trace"))
        logging::core::get()->set_filter(logging::trivial::severity >= boost::log::trivial::trace);
    else if (boost::iequals(lvl, L"debug"))
        logging::core::get()->set_filter(logging::trivial::severity >= boost::log::trivial::debug);
    else if (boost::iequals(lvl, L"info"))
        logging::core::get()->set_filter(logging::trivial::severity >= boost::log::trivial::info);
    else if (boost::iequals(lvl, L"warning"))
        logging::core::get()->set_filter(logging::trivial::severity >= boost::log::trivial::warning);
    else if (boost::iequals(lvl, L"error"))
        logging::core::get()->set_filter(logging::trivial::severity >= boost::log::trivial::error);
    else if (boost::iequals(lvl, L"fatal"))
        logging::core::get()->set_filter(logging::trivial::severity >= boost::log::trivial::fatal);
    else
        return false;

    current_log_level = lvl;
    return true;
}

std::wstring& get_log_level() { return current_log_level; }

}} // namespace caspar::log
