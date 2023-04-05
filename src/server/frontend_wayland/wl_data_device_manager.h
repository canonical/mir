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

#include "wayland_wrapper.h"

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

class WlDataDeviceManager : public wayland::DataDeviceManager::Global
{
public:
    WlDataDeviceManager(
        struct wl_display* display,
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<scene::Clipboard> const& clipboard,
        std::shared_ptr<DragIconController> drag_icon_controller);
    ~WlDataDeviceManager();

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<scene::Clipboard> const clipboard;
    std::shared_ptr<DragIconController> const drag_icon_controller;

    void bind(wl_resource* new_resource) override;

    class Instance : wayland::DataDeviceManager
    {
    public:
        Instance(wl_resource* new_resource, WlDataDeviceManager* manager);

    private:
        void create_data_source(wl_resource* new_resource) override;
        void get_data_device(wl_resource* new_data_device, wl_resource* seat) override;

        WlDataDeviceManager* const manager;
    };
};
}
}

#endif // MIR_FRONTEND_WL_DATA_DEVICE_MANAGER_H_
