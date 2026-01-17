#pragma once

#include <map>
#include <string>

namespace caspar { namespace http {

struct HTTPResponse
{
    unsigned int                       status_code;
    std::string                        status_message;
    std::map<std::string, std::string> headers;
    std::string                        body;
};

HTTPResponse request(const std::string& host, const std::string& port, const std::string& path);

// URL-encode a file path, preserving '/' and '\' as path separators.
// Encodes special characters in each path component per RFC 3986.
std::string url_encode_path(const std::string& path);

}} // namespace caspar::http
