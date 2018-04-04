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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "data_device_manager.h"

namespace mf = mir::frontend;

namespace
{
struct DataSource : mf::wayland::DataSource
{
    DataSource(struct wl_client* client, struct wl_resource* parent, uint32_t id) :
        mf::wayland::DataSource{client, parent, id}
    {
    }

    void offer(std::string const& mime_type) override
    {
        (void)mime_type;
    }

    void destroy() override
    {
        wl_resource_destroy(resource);
    }

    void set_actions(uint32_t dnd_actions) override
    {
        (void)dnd_actions;
    }
};

struct DataDevice : mf::wayland::DataDevice
{
    DataDevice(struct wl_client* client, struct wl_resource* parent, uint32_t id) :
        mf::wayland::DataDevice(client, parent, id)
    {
    }

    void start_drag(
        std::experimental::optional<struct wl_resource*> const& source, struct wl_resource* origin,
        std::experimental::optional<struct wl_resource*> const& icon, uint32_t serial) override
    {
        (void)source, (void)origin, (void)icon, (void)serial;
    }

    void set_selection(std::experimental::optional<struct wl_resource*> const& source, uint32_t serial) override
    {
        (void)source, (void)serial;
    }

    void release() override
    {
        wl_resource_destroy(resource);
    }
};
}

mf::DataDeviceManager::DataDeviceManager(struct wl_display* display) :
    wayland::DataDeviceManager(display, 3)
{
}

void mf::DataDeviceManager::create_data_source(struct wl_client* client, struct wl_resource* resource, uint32_t id)
{
    (void)client, (void)resource, (void)id;
    new DataSource{client, resource, id};
}

void mf::DataDeviceManager::get_data_device(
    struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat)
{
    (void)seat;
    new DataDevice{client, resource, id};
}
