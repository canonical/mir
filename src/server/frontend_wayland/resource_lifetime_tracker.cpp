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

#include "resource_lifetime_tracker.h"

#include <wayland-server-core.h>
#include <boost/throw_exception.hpp>

namespace mf = mir::frontend;

namespace
{
struct DestructionShim
{
    /// ResourceLifetimeTracker can not be the destruction shim itself because it is not standard layout
    mf::ResourceLifetimeTracker* tracker;
    wl_listener destruction_listener;
};

static_assert(
    std::is_standard_layout<DestructionShim>::value,
    "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");
}

auto mf::ResourceLifetimeTracker::from(wl_resource* resource) -> ResourceLifetimeTracker*
{
    if (!resource)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("ResourceLifetimeTracker created from null resource"));
    }
    if (auto notifier = wl_resource_get_destroy_listener(resource, &on_destroyed))
    {
        // We already have a shim for this resource
        DestructionShim* shim = wl_container_of(notifier, shim, destruction_listener);
        return shim->tracker;
    }
    else
    {
        return new ResourceLifetimeTracker{resource};
    }
}

mf::ResourceLifetimeTracker::ResourceLifetimeTracker(wl_resource* resource)
    : resource{resource}
{
    auto const shim = new DestructionShim{this, {}};
    shim->destruction_listener.notify = &on_destroyed;
    wl_resource_add_destroy_listener(resource, &shim->destruction_listener);
}

void mf::ResourceLifetimeTracker::on_destroyed(wl_listener* listener, void*)
{
    DestructionShim* shim;
    shim = wl_container_of(listener, shim, destruction_listener);
    delete shim->tracker;
    delete shim;
}
