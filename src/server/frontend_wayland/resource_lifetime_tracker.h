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

#ifndef MIR_FRONTEND_RESOURCE_LIFETIME_TRACKER_H_
#define MIR_FRONTEND_RESOURCE_LIFETIME_TRACKER_H_

#include <mir/wayland/lifetime_tracker.h>

struct wl_resource;
struct wl_listener;

namespace mir
{
namespace frontend
{
/// A lifetime tracker that wraps an arbitrary resource. Generated wrappers already have LifetimeTrackers, so this is
/// only needed when a resource is not managed by generated code.
class ResourceLifetimeTracker: public wayland::LifetimeTracker
{
public:
    static auto from(wl_resource* resource) -> ResourceLifetimeTracker*;
    operator wl_resource*() const { return resource; }

private:
    ResourceLifetimeTracker(wl_resource* resource);
    static void on_destroyed(wl_listener* listener, void*);

    wl_resource* resource;
};
}
}

#endif // MIR_FRONTEND_RESOURCE_LIFETIME_TRACKER_H_
