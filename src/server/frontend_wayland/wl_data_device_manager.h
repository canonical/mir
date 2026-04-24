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

#ifndef MIR_FRONTEND_WL_DATA_DEVICE_MANAGER_H_
#define MIR_FRONTEND_WL_DATA_DEVICE_MANAGER_H_

#include "wayland.h"
#include "client.h"
#include <memory>

namespace mir
{
class Executor;
namespace scene
{
class Clipboard;
}

namespace frontend
{
class DragIconController;
class PointerInputDispatcher;

class WlDataDeviceManager : public wayland_rs::WlDataDeviceManagerImpl
{
public:
    WlDataDeviceManager(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::Clipboard> const& clipboard,
        std::shared_ptr<DragIconController> drag_icon_controller,
        std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher,
        std::shared_ptr<wayland_rs::Client> const& client);

    auto create_data_source() -> std::shared_ptr<wayland_rs::WlDataSourceImpl> override;
    auto get_data_device(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat) -> std::shared_ptr<wayland_rs::WlDataDeviceImpl> override;

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::Clipboard> const clipboard;
    std::shared_ptr<DragIconController> const drag_icon_controller;
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher;
    std::shared_ptr<wayland_rs::Client> client;
};
}
}

#endif // MIR_FRONTEND_WL_DATA_DEVICE_MANAGER_H_
