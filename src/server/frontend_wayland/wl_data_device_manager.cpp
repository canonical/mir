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
namespace mw = mir::wayland_rs;

mf::WlDataDeviceManager::WlDataDeviceManager(
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<ms::Clipboard> const& clipboard,
    std::shared_ptr<DragIconController> drag_icon_controller,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
    std::shared_ptr<wayland_rs::Client> const& client) :
    wayland_executor{wayland_executor},
    clipboard{clipboard},
    drag_icon_controller{std::move(drag_icon_controller)},
    pointer_input_dispatcher{std::move(pointer_input_dispatcher)},
    client{client}
{
}

auto mf::WlDataDeviceManager::create_data_source() -> std::shared_ptr<wayland_rs::WlDataSourceImpl>
{
    return std::make_shared<WlDataSource>(
        wayland_executor,
        *clipboard);
}

auto mf::WlDataDeviceManager::get_data_device(mw::Weak<mw::WlSeatImpl> const& seat) -> std::shared_ptr<mw::WlDataDeviceImpl>
{
    auto const realseat = mf::WlSeat::from(&seat.value());
    return std::make_shared<WlDataDevice>(
        *wayland_executor,
        *clipboard,
        *realseat,
        pointer_input_dispatcher,
        drag_icon_controller,
        client);
}
