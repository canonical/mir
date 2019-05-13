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

#ifndef MIR_FRONTEND_DATA_DEVICE_H_
#define MIR_FRONTEND_DATA_DEVICE_H_

#include "wayland_wrapper.h"

namespace mir
{
namespace frontend
{
class DataDeviceManager : public wayland::DataDeviceManager::Global
{
public:
    using wayland::DataDeviceManager::Global::Global;
};

auto create_data_device_manager(struct wl_display* display) -> std::unique_ptr<DataDeviceManager>;
}
}

#endif //MIR_FRONTEND_DATA_DEVICE_H_
