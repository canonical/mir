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

#include "mir/wayland/wayland_base.h"
#include "mir/log.h"

#include <map>
#include <boost/throw_exception.hpp>
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

struct mw::LifetimeTracker::Impl
{
    std::shared_ptr<bool> destroyed{nullptr};
    std::map<DestroyListenerId, std::function<void()>> destroy_listeners;
    DestroyListenerId last_id{0};
};

mw::LifetimeTracker::LifetimeTracker()
{
}

mw::LifetimeTracker::~LifetimeTracker()
{
    mark_destroyed();
}

auto mw::LifetimeTracker::destroyed_flag() const -> std::shared_ptr<bool const>
{
    if (!impl)
    {
        impl = std::make_unique<Impl>();
    }
    if (!impl->destroyed)
    {
        impl->destroyed = std::make_shared<bool>(false);
    }
    return impl->destroyed;
}

auto mw::LifetimeTracker::add_destroy_listener(std::function<void()> listener) const -> DestroyListenerId
{
    if (!impl)
    {
        impl = std::make_unique<Impl>();
    }
    auto const id = DestroyListenerId{impl->last_id.as_value() + 1};
    impl->last_id = id;
    impl->destroy_listeners[id] = listener;
    return id;
}

void mw::LifetimeTracker::remove_destroy_listener(DestroyListenerId id) const
{
    if (impl)
    {
        impl->destroy_listeners.erase(id);
    }
}

void mw::LifetimeTracker::mark_destroyed() const
{
    if (impl)
    {
        auto const local_listeners = std::move(impl->destroy_listeners);
        impl->destroy_listeners.clear();
        for (auto const& listener : local_listeners)
        {
            listener.second();
        }
        if (impl->destroyed)
        {
            *impl->destroyed = true;
        }
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
    wl_client_post_implementation_error(client, "Mir internal error processing %s request", method_name);

    ::mir::log(
        ::mir::logging::Severity::warning,
        "frontend:Wayland",
        std::current_exception(),
        std::string() + "Exception processing " + method_name + " request");
}
