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

#include <mir/wayland/resource.h>
#include <mir/wayland/client.h>

#include <boost/throw_exception.hpp>
#include <wayland-server-core.h>

namespace mw = mir::wayland;

mw::Resource::Resource(wl_resource* resource)
    : owned_client{Client::shared_from(wl_resource_get_client(resource))},
      resource{resource},
      client{owned_client.get()}
{
    if (resource == nullptr)
    {
        BOOST_THROW_EXCEPTION((std::bad_alloc{}));
    }
}

mw::Resource::~Resource()
{
    // Run destroy listeners before client is dropped
    mark_destroyed();
}
