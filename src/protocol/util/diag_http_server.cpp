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

#include "../StdAfx.h"

#include "diag_http_server.h"

#include <common/log.h>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/read_until.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/streambuf.hpp>
#include <boost/asio/write.hpp>

#include <algorithm>
#include <memory>
#include <sstream>
#include <vector>

using tcp = boost::asio::ip::tcp;

namespace caspar { namespace protocol { namespace diag {

namespace {

// A single-page viewer: connects to /events with EventSource and draws one scrolling line-graph per
// value, per diagnostics graph, entirely in <canvas> - the server only ever sends JSON numbers.
const char* VIEWER_HTML = R"HTML(<!DOCTYPE html>
<html lang="en">
<head>
<meta charset="UTF-8">
<title>CasparCG Diagnostics</title>
<style>
  body { margin: 0; padding: 12px; background: #111; color: #eee; font-family: monospace; }
  .graph { border: 1px solid #333; margin-bottom: 12px; }
  .graph h2 { margin: 0; padding: 4px 8px; font-size: 13px; background: #222; }
  canvas { display: block; width: 100%; height: 80px; }
</style>
</head>
<body>
<div id="graphs"></div>
<script>
  var HISTORY = 300;
  var series = {};   // graphKey -> { canvas, ctx, values: { name: [numbers...] }, colors: { name: cssColor } }

  function colorToCss(c) {
    if (typeof c !== "number") return "#0f0";
    var r = (c >>> 24) & 255, g = (c >>> 16) & 255, b = (c >>> 8) & 255, a = c & 255;
    return "rgba(" + r + "," + g + "," + b + "," + (a / 255) + ")";
  }

  function ensureGraph(key, text) {
    if (series[key]) return series[key];

    var container = document.createElement("div");
    container.className = "graph";
    var h2 = document.createElement("h2");
    h2.textContent = text || key;
    var canvas = document.createElement("canvas");
    canvas.width = 900;
    canvas.height = 80;
    container.appendChild(h2);
    container.appendChild(canvas);
    document.getElementById("graphs").appendChild(container);

    series[key] = { h2: h2, canvas: canvas, ctx: canvas.getContext("2d"), values: {}, colors: {} };
    return series[key];
  }

  function draw(g) {
    var ctx = g.ctx, w = g.canvas.width, h = g.canvas.height;
    ctx.clearRect(0, 0, w, h);

    Object.keys(g.values).forEach(function (name) {
      var vals = g.values[name];
      ctx.strokeStyle = g.colors[name] || "#0f0";
      ctx.beginPath();
      for (var i = 0; i < vals.length; i++) {
        var x = (i / (HISTORY - 1)) * w;
        var y = h - Math.max(0, Math.min(1, vals[i])) * h;
        if (i === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y);
      }
      ctx.stroke();
    });
  }

  function applySnapshot(snapshot) {
    (snapshot.graphs || []).forEach(function (graph, index) {
      var key = "graph-" + index;
      var g = ensureGraph(key, graph.text);
      g.h2.textContent = graph.text || key;

      Object.keys(graph.values || {}).forEach(function (name) {
        if (!g.values[name]) g.values[name] = [];
        g.values[name].push(graph.values[name]);
        if (g.values[name].length > HISTORY) g.values[name].shift();
        if (graph.colors && graph.colors[name] !== undefined) g.colors[name] = colorToCss(graph.colors[name]);
      });

      draw(g);
    });
  }

  var source = new EventSource("/events");
  source.onmessage = function (e) { applySnapshot(JSON.parse(e.data)); };
</script>
</body>
</html>
)HTML";

std::string http_response(const std::string& content_type, const std::string& body)
{
    std::ostringstream os;
    os << "HTTP/1.1 200 OK\r\n"
       << "Content-Type: " << content_type << "\r\n"
       << "Content-Length: " << body.size() << "\r\n"
       << "Access-Control-Allow-Origin: *\r\n"
       << "Connection: close\r\n"
       << "\r\n"
       << body;
    return os.str();
}

std::string sse_headers()
{
    return "HTTP/1.1 200 OK\r\n"
           "Content-Type: text/event-stream\r\n"
           "Cache-Control: no-cache\r\n"
           "Access-Control-Allow-Origin: *\r\n"
           "Connection: keep-alive\r\n"
           "\r\n";
}

std::string sse_frame(const std::string& json) { return "data: " + json + "\n\n"; }

} // namespace

struct diag_http_server::impl : public std::enable_shared_from_this<impl>
{
    std::shared_ptr<boost::asio::io_context> io_context_;
    tcp::acceptor                            acceptor_;
    std::function<std::string()>             snapshot_provider_;
    std::chrono::milliseconds                push_interval_;
    boost::asio::steady_timer                push_timer_;
    std::vector<std::shared_ptr<tcp::socket>> sse_clients_;

    impl(std::shared_ptr<boost::asio::io_context> io_context,
         uint16_t                                 port,
         std::function<std::string()>             snapshot_provider,
         std::chrono::milliseconds                push_interval)
        : io_context_(std::move(io_context))
        , acceptor_(*io_context_, tcp::endpoint(tcp::v4(), port))
        , snapshot_provider_(std::move(snapshot_provider))
        , push_interval_(push_interval)
        , push_timer_(*io_context_)
    {
    }

    void start()
    {
        accept();
        schedule_push();
    }

    void stop()
    {
        boost::system::error_code ec;
        acceptor_.close(ec);
        push_timer_.cancel();
        sse_clients_.clear();
    }

    void accept()
    {
        auto socket = std::make_shared<tcp::socket>(*io_context_);
        auto self   = shared_from_this();

        acceptor_.async_accept(*socket, [this, self, socket](const boost::system::error_code& ec) {
            if (!ec)
                read_request(socket);

            if (acceptor_.is_open())
                accept();
        });
    }

    void read_request(const std::shared_ptr<tcp::socket>& socket)
    {
        auto self   = shared_from_this();
        auto buffer = std::make_shared<boost::asio::streambuf>();

        boost::asio::async_read_until(
            *socket, *buffer, "\r\n", [this, self, socket, buffer](const boost::system::error_code& ec, size_t) {
                if (ec)
                    return;

                std::istream        is(buffer.get());
                std::string         method;
                std::string         path;
                is >> method >> path;

                handle_request(socket, path);
            });
    }

    void handle_request(const std::shared_ptr<tcp::socket>& socket, const std::string& path)
    {
        if (path == "/events") {
            serve_events(socket);
            return;
        }

        std::string response;
        if (path == "/snapshot")
            response = http_response("application/json", snapshot_provider_());
        else
            response = http_response("text/html; charset=utf-8", VIEWER_HTML);

        auto self = shared_from_this();
        boost::asio::async_write(
            *socket,
            boost::asio::buffer(response),
            [self, socket](const boost::system::error_code&, size_t) {
                boost::system::error_code ec;
                socket->shutdown(tcp::socket::shutdown_both, ec);
            });
    }

    void serve_events(const std::shared_ptr<tcp::socket>& socket)
    {
        auto self    = shared_from_this();
        auto headers = std::make_shared<std::string>(sse_headers());

        boost::asio::async_write(
            *socket, boost::asio::buffer(*headers), [this, self, socket, headers](const boost::system::error_code& ec, size_t) {
                if (!ec)
                    sse_clients_.push_back(socket);
            });
    }

    void schedule_push()
    {
        auto self = shared_from_this();
        push_timer_.expires_after(push_interval_);
        push_timer_.async_wait([this, self](const boost::system::error_code& ec) {
            if (ec)
                return;

            push_to_clients();
            schedule_push();
        });
    }

    void push_to_clients()
    {
        if (sse_clients_.empty())
            return;

        auto frame = std::make_shared<std::string>(sse_frame(snapshot_provider_()));
        auto self  = shared_from_this();

        auto clients = sse_clients_;
        for (auto& client : clients) {
            boost::asio::async_write(
                *client, boost::asio::buffer(*frame), [this, self, client, frame](const boost::system::error_code& ec, size_t) {
                    if (ec) {
                        sse_clients_.erase(std::remove(sse_clients_.begin(), sse_clients_.end(), client),
                                            sse_clients_.end());
                    }
                });
        }
    }
};

diag_http_server::diag_http_server(std::shared_ptr<boost::asio::io_context> io_context,
                                    uint16_t                                 port,
                                    std::function<std::string()>             snapshot_provider,
                                    std::chrono::milliseconds                push_interval)
    : impl_(std::make_shared<impl>(std::move(io_context), port, std::move(snapshot_provider), push_interval))
{
    // impl derives from enable_shared_from_this, so it must already be owned by a shared_ptr (impl_,
    // above) before any of its methods can call shared_from_this() - hence start() is called here rather
    // than from impl's own constructor.
    impl_->start();
}

diag_http_server::~diag_http_server()
{
    if (impl_)
        impl_->stop();
}

}}} // namespace caspar::protocol::diag
