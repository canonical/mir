/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#include "deleted_for_resource.h"

#include <wayland-server-core.h>

namespace
{
class DestructionShim
{
public:
    static std::shared_ptr<bool> flag_for_resource(wl_resource* resource)
    {
        DestructionShim* shim;

        if (auto notifier = wl_resource_get_destroy_listener(resource, &on_destroyed))
        {
            // We already have a flag for this resource
            shim = wl_container_of(notifier, shim, destruction_listener);
        }
        else
        {
            shim = new DestructionShim{resource};
        }
        return shim->destroyed;
    }
private:
    DestructionShim(wl_resource* resource)
        : destroyed{std::make_shared<bool>(false)}
    {
        destruction_listener.notify = &on_destroyed;
        wl_resource_add_destroy_listener(resource, &destruction_listener);
    }

    static void on_destroyed(wl_listener* listener, void*)
    {
        DestructionShim* shim;

        shim = wl_container_of(listener, shim, destruction_listener);
        *shim->destroyed = true;
        delete shim;
    }

    std::shared_ptr<bool> const destroyed;
    wl_listener destruction_listener;
};
static_assert(
    std::is_standard_layout<DestructionShim>::value,
    "DestructionShim must be Standard Layout for wl_container_of to be defined behaviour");
}

/**
 * Get a flag that will be set to true once \param resource is destroyed.
 *
 * \param resource [in] The resource to monitor for destruction.
 * \return A std::shared_ptr<bool> that becomes true once the resource is destroyed.
 *
 * \note    Not threadsafe! Specifically, the value of the flag is only guaranteed to be correct
 *          on the thread handling the Wayland event loop.
 */
std::shared_ptr<bool> mir::frontend::wayland::deleted_flag_for_resource(wl_resource* resource)
{
    return DestructionShim::flag_for_resource(resource);
}
