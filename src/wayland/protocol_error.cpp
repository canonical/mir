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

#include <mir/wayland/protocol_error.h>
#include <mir/log.h>

#include <wayland-server-core.h>

namespace mw = mir::wayland;

mw::ProtocolError::ProtocolError(
    wl_resource* source,
    uint32_t code,
    char const* fmt, ...) :
        std::runtime_error{"Client protocol error"},
        source{source},
        error_code{code}
{
    va_list va;
    char message[1024];

    va_start(va, fmt);
    auto const max = sizeof(message) - 1;
    auto const len = vsnprintf(message, max, fmt, va);
    va_end(va);

    error_message = std::string(std::begin(message), std::begin(message) + len);
}

auto mw::ProtocolError::message() const -> char const*
{
    return error_message.c_str();
}

auto mw::ProtocolError::resource() const -> wl_resource*
{
    return source;
}

auto mw::ProtocolError::code() const -> uint32_t
{
    return error_code;
}

void mw::internal_error_processing_request(wl_client* client, char const* method_name)
{
    wl_client_post_implementation_error(client, "Mir internal error processing %s request", method_name);

    ::mir::log(
        ::mir::logging::Severity::warning,
        "frontend:Wayland",
        std::current_exception(),
        std::string() + "Exception processing " + method_name + " request");
}

void mw::tried_to_send_unsupported_event(wl_client* client, wl_resource* resource, char const* event, int required_version)
{
    wl_client_post_implementation_error(
        client,
        "Mir tried to send %s@%u.%s to object with version %d (requires version %d)",
        wl_resource_get_class(resource),
        wl_resource_get_id(resource),
        event,
        wl_resource_get_version(resource),
        required_version);

    log_critical(
        "Tried to send {}@{}.{} to object with version {} (requires version {}",
        wl_resource_get_class(resource),
        wl_resource_get_id(resource),
        event,
        wl_resource_get_version(resource),
        required_version);
}
