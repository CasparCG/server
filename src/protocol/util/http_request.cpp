#include "http_request.h"

#include <common/except.h>

#include <boost/asio.hpp>
#include <iomanip>
#include <sstream>
#include <string>

namespace caspar { namespace http {

HTTPResponse request(const std::string& host, const std::string& port, const std::string& path)
{
    using boost::asio::ip::tcp;
    using namespace boost;

    HTTPResponse res;

    asio::io_service io_service;

    // Get a list of endpoints corresponding to the server name.
    tcp::resolver           resolver(io_service);
    tcp::resolver::query    query(host, port, boost::asio::ip::resolver_query_base::numeric_service);
    tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

    // Try each endpoint until we successfully establish a connection.
    tcp::socket               socket(io_service);
    boost::system::error_code error;
    asio::connect(socket, endpoint_iterator, error);
    if (error == asio::error::connection_refused) {
        res.status_code    = 503;
        res.status_message = "Connection refused";
        return res;
    }
    if (error) {
        CASPAR_THROW_EXCEPTION(io_error() << msg_info(error.message()));
    }

    // Form the request. We specify the "Connection: close" header so that the
    // server will close the socket after transmitting the response. This will
    // allow us to treat all data up until the EOF as the content.
    asio::streambuf request;
    std::ostream    request_stream(&request);
    request_stream << "GET " << path << " HTTP/1.0\r\n";
    request_stream << "Host: " << host << ":" << port << "\r\n";
    request_stream << "Accept: */*\r\n";
    request_stream << "Connection: close\r\n\r\n";

    // Send the request.
    asio::write(socket, request);

    // Read the response status line. The response streambuf will automatically
    // grow to accommodate the entire line. The growth may be limited by passing
    // a maximum size to the streambuf constructor.
    asio::streambuf response;
    asio::read_until(socket, response, "\r\n");

    // Check that response is OK.
    std::istream response_stream(&response);
    std::string  http_version;
    response_stream >> http_version;
    response_stream >> res.status_code;
    std::getline(response_stream, res.status_message);

    if (!response_stream || http_version.substr(0, 5) != "HTTP/") {
        // TODO
        CASPAR_THROW_EXCEPTION(io_error() << msg_info("Invalid Response"));
    }

    if (res.status_code < 200 || res.status_code >= 300) {
        // TODO
        CASPAR_THROW_EXCEPTION(io_error() << msg_info("Invalid Response"));
    }

    // Read the response headers, which are terminated by a blank line.
    asio::read_until(socket, response, "\r\n\r\n");

    // Process the response headers.
    std::string header;
    while (std::getline(response_stream, header) && header != "\r") {
        // TODO
    }

    std::stringstream body;

    // Write whatever content we already have to output.
    if (response.size() > 0) {
        body << &response;
    }

    // Read until EOF, writing data to output as we go.
    while (asio::read(socket, response, asio::transfer_at_least(1), error)) {
        body << &response;
    }

    if (error != asio::error::eof) {
        CASPAR_THROW_EXCEPTION(io_error() << msg_info(error.message()));
    }

    res.body = body.str();

    return res;
}

std::string url_encode(const std::string& str)
{
    std::stringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (auto c : str) {
        if (std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
        } else {
            escaped << std::uppercase << '%' << std::setw(2) << int((unsigned char)c) << std::nouppercase;
        }
    }

    return escaped.str();
}

}} // namespace caspar::http
