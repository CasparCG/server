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

#include "boost/date_time/gregorian/gregorian.hpp"
#include <boost/algorithm/string/predicate.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/locale.hpp>
#include <boost/log/attributes/function.hpp>
#include <boost/log/core.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/async_frontend.hpp>
#include <boost/log/sinks/basic_sink_backend.hpp>
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

namespace logging  = boost::log;
namespace src      = boost::log::sources;
namespace sinks    = boost::log::sinks;
namespace keywords = boost::log::keywords;

using namespace boost::placeholders;

namespace caspar { namespace log {

std::wstring current_level;

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

void remove_all_sinks()
{
    boost::log::core::get()->flush();
    boost::log::core::get()->remove_all_sinks();
}

struct node_sink_backend
    : public boost::log::sinks::basic_sink_backend<sinks::combine_requirements<sinks::concurrent_feeding>::type>
{
    node_sink_backend(std::shared_ptr<log_receiver_emitter> sink)
        : sink_(std::move(sink))
        , myEpoch(boost::posix_time::from_time_t(0))
    {
    }

    void consume(logging::record_view const& rec)
    {
        std::string level = boost::log::trivial::to_string(
            boost::log::extract<boost::log::trivial::severity_level>("Severity", rec).get());

        boost::posix_time::ptime timestamp_ns =
            boost::log::extract<boost::posix_time::ptime>("TimestampMillis", rec).get();
        std::int64_t timestamp_millis = (timestamp_ns - myEpoch).ticks() / 1000;

        std::string message = u8(rec[boost::log::expressions::message].get<std::wstring>());

        sink_->send_log(timestamp_millis, message, level);
    }

  private:
    boost::posix_time::ptime myEpoch;

    std::shared_ptr<log_receiver_emitter> sink_;
};

void add_nodejs_sink(std::shared_ptr<log_receiver_emitter> sink)
{
    boost::log::add_common_attributes();
    // boost::log::core::get()->add_global_attribute("NativeThreadId",
    // boost::log::attributes::make_function(&std::this_thread::get_id));
    boost::log::core::get()->add_global_attribute("TimestampMillis", boost::log::attributes::make_function([] {
                                                      return boost::posix_time::microsec_clock::local_time();
                                                  }));

    auto node_sink  = boost::make_shared<node_sink_backend>(sink);
    auto boost_sink = boost::make_shared<sinks::asynchronous_sink<node_sink_backend>>(node_sink);

    logging::core::get()->add_sink(boost_sink);
}

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

    // TODO is this a race condition?
    current_level = lvl;
    return true;
}

std::wstring& get_log_level() { return current_level; }

}} // namespace caspar::log
