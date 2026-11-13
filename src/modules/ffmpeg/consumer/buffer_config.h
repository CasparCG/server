#pragma once

#include <common/except.h>

#include <boost/algorithm/string/predicate.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/property_tree/ptree.hpp>

#include <string>
#include <utility>
#include <vector>

namespace caspar { namespace ffmpeg {

struct buffer_config
{
  private:
    bool realtime_;

    void require_realtime() const
    {
        if (!realtime_) {
            CASPAR_THROW_EXCEPTION(
                user_error() << msg_info_t("Buffer options require STREAM or an XML consumer with realtime=true."));
        }
    }

  public:
    int frames;
    int packets;

    explicit buffer_config(bool realtime)
        : realtime_(realtime)
        , frames(realtime ? 6 : 64)
        , packets(realtime ? 16 : 128)
    {
    }

    static int parse(const std::wstring& value, const char* name, int maximum)
    {
        int result = 0;
        try {
            result = boost::lexical_cast<int>(value);
        } catch (const boost::bad_lexical_cast&) {
            CASPAR_THROW_EXCEPTION(user_error() << msg_info_t(std::string(name) + " must be an integer."));
        }
        if (result < 1 || result > maximum) {
            CASPAR_THROW_EXCEPTION(user_error() << msg_info_t(std::string(name) + " must be between 1 and " +
                                                              std::to_string(maximum) + "."));
        }
        return result;
    }

    void consume_options(std::vector<std::wstring>& args)
    {
        std::vector<std::wstring> remaining;
        for (std::size_t n = 0; n < args.size(); ++n) {
            const bool frame_option  = boost::iequals(args[n], L"-frame-buffer");
            const bool packet_option = boost::iequals(args[n], L"-packet-buffer");
            if (!frame_option && !packet_option) {
                remaining.push_back(args[n]);
                continue;
            }
            require_realtime();
            const auto* name = frame_option ? "frame-buffer" : "packet-buffer";
            if (++n == args.size()) {
                CASPAR_THROW_EXCEPTION(user_error() << msg_info_t(std::string(name) + " requires a value."));
            }
            (frame_option ? frames : packets) = parse(args[n], name, frame_option ? 64 : 1024);
        }
        args = std::move(remaining);
    }

    void read_config(const boost::property_tree::wptree& ptree)
    {
        if (const auto value = ptree.get_optional<std::wstring>(L"frame-buffer")) {
            require_realtime();
            frames = parse(*value, "frame-buffer", 64);
        }
        if (const auto value = ptree.get_optional<std::wstring>(L"packet-buffer")) {
            require_realtime();
            packets = parse(*value, "packet-buffer", 1024);
        }
    }
};

}} // namespace caspar::ffmpeg
