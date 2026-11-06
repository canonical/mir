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

#include <stdexcept>
#include <utility>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mw = mir::wayland;

mf::WlDataDeviceManager::WlDataDeviceManager(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::DataDeviceManagerMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<mir::Executor> const& wayland_executor,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<DragIconController> drag_icon_controller,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher)
    : mw::DataDeviceManager{std::move(client), std::move(instance), object_id},
      wayland_executor{wayland_executor},
      clipboard{clipboard},
      drag_icon_controller{std::move(drag_icon_controller)},
      pointer_input_dispatcher{std::move(pointer_input_dispatcher)}
{
}

auto mf::create_wl_data_device_manager(
    std::shared_ptr<mw::Client> client,
    rust::Box<mw::DataDeviceManagerMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<DragIconController> drag_icon_controller,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher)
-> std::shared_ptr<mw::DataDeviceManager>
{
    return std::make_shared<WlDataDeviceManager>(
        std::move(client),
        std::move(instance),
        object_id,
        wayland_executor,
        clipboard,
        std::move(drag_icon_controller),
        std::move(pointer_input_dispatcher));
}

auto mf::WlDataDeviceManager::create_data_source(
    rust::Box<mw::DataSourceMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::DataSource>
{
    return std::make_shared<WlDataSource>(
        client,
        std::move(child_instance),
        child_object_id,
        wayland_executor,
        *clipboard);
}

auto mf::WlDataDeviceManager::get_data_device(
    mw::Weak<mw::Seat> const& seat,
    rust::Box<mw::DataDeviceMiddleware> child_instance,
    uint32_t child_object_id) -> std::shared_ptr<mw::DataDevice>
{
    auto const wl_seat = WlSeat::from(seat);
    if (!wl_seat)
    {
        throw std::logic_error{"client provided incorrect seat to wl_data_device_manager.get_data_device"};
    }

    return std::make_shared<WlDataDevice>(
        client,
        std::move(child_instance),
        child_object_id,
        *wayland_executor,
        *clipboard,
        *wl_seat,
        pointer_input_dispatcher,
        drag_icon_controller);
}
