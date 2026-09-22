#pragma once

#include <map>
#include <string>
#include <string_view>

namespace caspar { namespace http {

struct HTTPResponse
{
    unsigned int                       status_code;
    std::string                        status_message;
    std::map<std::string, std::string> headers;
    std::string                        body;
};

HTTPResponse request(const std::string& host, const std::string& port, const std::string& path);

// URL-encode a file path. Normalizes '\' to '/' before encoding, so lookups match
// regardless of client OS; '/' is percent-encoded like any other character.
std::string url_encode_path(std::string_view path);

}} // namespace caspar::http
