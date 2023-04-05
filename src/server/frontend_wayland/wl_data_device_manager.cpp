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

#include "wl_data_device_manager.h"
#include "wl_data_device.h"
#include "wl_data_source.h"
#include "wl_seat.h"

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

mf::WlDataDeviceManager::WlDataDeviceManager(
    struct wl_display* display,
    std::shared_ptr<mir::Executor> const& wayland_executor,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<DragIconController> drag_icon_controller) :
    Global(display, Version<3>()),
    wayland_executor{wayland_executor},
    clipboard{clipboard},
    drag_icon_controller{std::move(drag_icon_controller)}
{
}

mf::WlDataDeviceManager::~WlDataDeviceManager()
{
}

mf::WlDataDeviceManager::Instance::Instance(wl_resource* new_resource, WlDataDeviceManager* manager)
    : wayland::DataDeviceManager(new_resource, Version<3>()),
      manager{manager}
{
}

void mf::WlDataDeviceManager::Instance::create_data_source(wl_resource* new_data_source)
{
    new WlDataSource{new_data_source, manager->wayland_executor, *manager->clipboard};
}

void mf::WlDataDeviceManager::Instance::get_data_device(wl_resource* new_data_device, wl_resource* seat)
{
    auto const realseat = mf::WlSeat::from(seat);
    new WlDataDevice{new_data_device, *manager->wayland_executor, *manager->clipboard, *realseat};
}

void mf::WlDataDeviceManager::bind(wl_resource* new_resource)
{
    new WlDataDeviceManager::Instance{new_resource, this};
}
