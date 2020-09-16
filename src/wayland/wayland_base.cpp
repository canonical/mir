/*
 * Copyright © 2019 Canonical Ltd.
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
 *
 * Authored By: William Wold <william.wold@canonical.com>
 */

#include <boost/throw_exception.hpp>
#include <wayland-server-core.h>
#include "mir/log.h"

#include "mir/wayland/wayland_base.h"

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

mw::LifetimeTracker::~LifetimeTracker()
{
    if (destroyed)
    {
        *destroyed = true;
    }
}

auto mw::LifetimeTracker::destroyed_flag() const -> std::shared_ptr<bool>
{
    if (!destroyed)
    {
        destroyed = std::make_shared<bool>(false);
    }
    return destroyed;
}

void mw::LifetimeTracker::mark_destroyed() const
{
    if (destroyed)
    {
        *destroyed = true;
    }
}

mw::Resource::Resource()
{
}

mw::Global::Global(wl_global* global)
    : global{global}
{
    if (global == nullptr)
    {
        BOOST_THROW_EXCEPTION((std::bad_alloc{}));
    }
}

mw::Global::~Global()
{
    wl_global_destroy(global);
}

void mw::internal_error_processing_request(wl_client* client, char const* method_name)
{
#if (WAYLAND_VERSION_MAJOR > 1 || (WAYLAND_VERSION_MAJOR == 1 && WAYLAND_VERSION_MINOR > 16))
    wl_client_post_implementation_error(client, "Mir internal error processing %s request", method_name);
#else
    wl_client_post_no_memory(client);
#endif
    ::mir::log(
        ::mir::logging::Severity::warning,
        "frontend:Wayland",
        std::current_exception(),
        std::string() + "Exception processing " + method_name + " request");
}
