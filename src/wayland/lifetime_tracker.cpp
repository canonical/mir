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

#include <mir/wayland/lifetime_tracker.h>

#include <map>

namespace mw = mir::wayland;

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
