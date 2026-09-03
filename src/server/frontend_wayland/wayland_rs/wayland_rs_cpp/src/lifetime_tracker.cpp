/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
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

#include "lifetime_tracker.h"

#include <map>
#include <mutex>

namespace mrs = mir::wayland_rs;

struct mrs::LifetimeTracker::Impl
{
    std::mutex mutable mutex;
    std::shared_ptr<bool> destroyed{nullptr};
    std::map<uint64_t, std::function<void()>> destroy_listeners;
    uint64_t last_id{0};
    bool already_destroyed{false};
};

mrs::LifetimeTracker::LifetimeTracker()
    : impl{std::make_unique<Impl>()}
{
}

mrs::LifetimeTracker::~LifetimeTracker()
{
    mark_destroyed();
}

auto mrs::LifetimeTracker::destroyed_flag() const -> std::shared_ptr<bool const>
{
    std::lock_guard lock{impl->mutex};
    if (!impl->destroyed)
    {
        impl->destroyed = std::make_shared<bool>(false);
    }
    return impl->destroyed;
}

auto mrs::LifetimeTracker::add_destroy_listener(std::function<void()> listener) const -> uint64_t
{
    std::lock_guard lock{impl->mutex};
    auto const id = impl->last_id + 1;
    impl->last_id = id;
    impl->destroy_listeners[id] = std::move(listener);
    return id;
}

void mrs::LifetimeTracker::remove_destroy_listener(uint64_t id) const
{
    std::lock_guard lock{impl->mutex};
    impl->destroy_listeners.erase(id);
}

void mrs::LifetimeTracker::mark_destroyed() const
{
    decltype(impl->destroy_listeners) local_listeners;
    {
        std::lock_guard lock{impl->mutex};
        if (impl->already_destroyed)
            return;
        impl->already_destroyed = true;
        local_listeners = std::move(impl->destroy_listeners);
        impl->destroy_listeners.clear();
    }
    // Call listeners outside the lock to avoid deadlocks
    for (auto const& listener : local_listeners)
    {
        listener.second();
    }
    {
        std::lock_guard lock{impl->mutex};
        if (impl->destroyed)
        {
            *impl->destroyed = true;
        }
    }
}
