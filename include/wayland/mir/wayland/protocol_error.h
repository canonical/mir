/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_WAYLAND_PROTOCOL_ERROR_H_
#define MIR_WAYLAND_PROTOCOL_ERROR_H_

#include <stdexcept>
#include <cstdint>
#include <string>

struct wl_resource;
struct wl_client;

namespace mir
{
namespace wayland
{
/// For when the protocol does not provide an appropriate error code
uint32_t const generic_error_code = -1;

/**
 * An exception type representing a Wayland protocol error
 *
 * Throwing one of these from a request handler will result in the client
 * being sent a \a code error on \a source, with the printf-style \a fmt string
 * populated as the message.:
 */
class ProtocolError : public std::runtime_error
{
public:
    [[gnu::format (printf, 4, 5)]]  // Format attribute counts the hidden this parameter
    ProtocolError(wl_resource* source, uint32_t code, char const* fmt, ...);

    auto message() const -> char const*;
    auto resource() const -> wl_resource*;
    auto code() const -> uint32_t;
private:
    std::string error_message;
    wl_resource* const source;
    uint32_t const error_code;
};

void internal_error_processing_request(wl_client* client, char const* method_name);
void tried_to_send_unsupported_event(wl_client* client, wl_resource* resource, char const* event, int required_version);
}
}

#endif // MIR_WAYLAND_PROTOCOL_ERROR_H_
