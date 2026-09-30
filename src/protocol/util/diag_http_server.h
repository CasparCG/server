/*
 * Copyright (c) 2026 Sveriges Television AB <info@casparcg.com>
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
 */

#pragma once

#include <boost/asio/io_context.hpp>

#include <chrono>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>

namespace caspar { namespace protocol { namespace diag {

// Minimal HTTP server exposing a diagnostics snapshot (see core/diagnostics/web_graph.h) for browser
// viewing or for other clients to subscribe to and render/process themselves:
//
//   GET /          -> a small static HTML/JS viewer; all rendering happens client-side in <canvas>
//   GET /snapshot  -> a single JSON snapshot, for clients that just want to poll
//   GET /events    -> a Server-Sent Events stream of JSON snapshots, pushed at push_interval
//
// This is intentionally just a GET-only HTTP/1.1 responder over raw asio - no TLS, no auth, no
// external HTTP library. It is meant to run on a trusted monitoring network only, same as OSC/AMCP.
class diag_http_server
{
  public:
    diag_http_server(std::shared_ptr<boost::asio::io_context> io_context,
                      uint16_t                                 port,
                      std::function<std::string()>             snapshot_provider,
                      std::chrono::milliseconds                push_interval = std::chrono::milliseconds(150));
    ~diag_http_server();

    diag_http_server(const diag_http_server&)            = delete;
    diag_http_server& operator=(const diag_http_server&) = delete;

  private:
    struct impl;
    std::shared_ptr<impl> impl_;
};

}}} // namespace caspar::protocol::diag
