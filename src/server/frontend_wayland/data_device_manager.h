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

#ifndef MIR_DATA_DEVICE_MANAGER_H
#define MIR_DATA_DEVICE_MANAGER_H

#include "generated/wayland_wrapper.h"

namespace mir
{
namespace frontend
{
class DataDeviceManager : wayland::DataDeviceManager
{
public:
    DataDeviceManager(struct wl_display* display);

    void create_data_source(struct wl_client* client, struct wl_resource* resource, uint32_t id) override;

    void get_data_device(
        struct wl_client* client, struct wl_resource* resource, uint32_t id, struct wl_resource* seat) override;
};
}
}

#endif //MIR_DATA_DEVICE_MANAGER_H
