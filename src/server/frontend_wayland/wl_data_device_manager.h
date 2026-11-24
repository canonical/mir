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

#include <cstdint>
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

class WlDataDeviceManager : public wayland::DataDeviceManager
{
public:
    WlDataDeviceManager(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::DataDeviceManagerMiddleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::Clipboard> const& clipboard,
        std::shared_ptr<DragIconController> drag_icon_controller,
        std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher);

    using wayland::DataDeviceManager::get_data_device;

private:
    auto create_data_source(
        rust::Box<wayland::DataSourceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::DataSource> override;

    auto get_data_device(
        wayland::Weak<wayland::Seat> const& seat,
        rust::Box<wayland::DataDeviceMiddleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::DataDevice> override;

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::Clipboard> const clipboard;
    std::shared_ptr<DragIconController> const drag_icon_controller;
    std::shared_ptr<PointerInputDispatcher> const pointer_input_dispatcher;
};

auto create_wl_data_device_manager(
    std::shared_ptr<wayland::Client> client,
    rust::Box<wayland::DataDeviceManagerMiddleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<scene::Clipboard> const& clipboard,
    std::shared_ptr<DragIconController> drag_icon_controller,
    std::shared_ptr<PointerInputDispatcher> pointer_input_dispatcher)
-> std::shared_ptr<wayland::DataDeviceManager>;

inline auto validate_dnd_actions(uint32_t dnd_actions) -> bool
{
    return (dnd_actions & ~(
        mir::wayland::DataDeviceManager::DndAction::none |
        mir::wayland::DataDeviceManager::DndAction::copy |
        mir::wayland::DataDeviceManager::DndAction::r_move |
        mir::wayland::DataDeviceManager::DndAction::ask)) == 0;
}

inline auto validate_dnd_action(uint32_t dnd_action) -> bool
{
    return dnd_action == mir::wayland::DataDeviceManager::DndAction::none ||
           dnd_action == mir::wayland::DataDeviceManager::DndAction::copy ||
           dnd_action == mir::wayland::DataDeviceManager::DndAction::r_move ||
           dnd_action == mir::wayland::DataDeviceManager::DndAction::ask;
}
}
}

#endif // MIR_FRONTEND_WL_DATA_DEVICE_MANAGER_H_
