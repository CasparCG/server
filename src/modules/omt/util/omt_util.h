/*
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

#include "../interop/libomt.h"

#include <protocol/amcp/amcp_command_context.h>

#include <mutex>
#include <string>
#include <vector>

namespace caspar { namespace omt {

// The subset of the libomt C API used by the producer/consumer, resolved dynamically
// so CasparCG still builds and starts without the OMT runtime installed.
struct omt_lib
{
    decltype(&::omt_discovery_getaddresses) discovery_getaddresses;

    decltype(&::omt_receive_create)  receive_create;
    decltype(&::omt_receive_destroy) receive_destroy;
    decltype(&::omt_receive)         receive;

    decltype(&::omt_send_create)     send_create;
    decltype(&::omt_send_destroy)    send_destroy;
    decltype(&::omt_send)            send;
    decltype(&::omt_send_getaddress) send_getaddress;
    decltype(&::omt_send_connections) send_connections;
};

// Loads (once) and returns the libomt runtime, or throws caspar::not_supported if it
// couldn't be found/loaded.
omt_lib* load_library();

// Serializes calls into omt_send_create/omt_receive_create (and their matching destroy
// calls) process-wide, with a short minimum spacing between them.
//
// Works around a libomt bug: the discovery name of every sender after the first one
// (within the same process) loses its case. Also guards against a real CasparCG-side
// race for calls made after startup.
std::unique_lock<std::mutex> serialize_create_call();

// Returns an ASCII-safe transliteration of `name` for use as an OMT sender name:
// diacritics are stripped, anything else non-ASCII is replaced with '_'.
//
// Works around a libomt bug: a non-ASCII sender name breaks that sender's discovery
// entirely once more than one sender exists in the same process. Only affects sending;
// a producer's connection target name is left as given, since it must match whatever
// the remote sender actually announces.
std::wstring make_ascii_safe_name(const std::wstring& name);

// Returns the list of OMT sources (Address Name) currently visible via discovery.
std::vector<std::string> get_current_sources();

// AMCP "OMT LIST" query command handler.
std::wstring list_command(protocol::amcp::command_context& ctx);

}} // namespace caspar::omt
