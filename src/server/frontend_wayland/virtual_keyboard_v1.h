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

#ifndef MIR_FRONTEND_VIRTUAL_KEYBOARD_V1_H
#define MIR_FRONTEND_VIRTUAL_KEYBOARD_V1_H

#include "virtual_keyboard_unstable_v1.h"
#include "client.h"

#include <cstdint>
#include <memory>

namespace mir
{
namespace input
{
class InputDeviceRegistry;
}
namespace frontend
{
auto create_virtual_keyboard_manager_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::VirtualKeyboardManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<input::InputDeviceRegistry> const& device_registry)
-> std::shared_ptr<wayland_rs::VirtualKeyboardManagerV1>;
}
}

#endif // MIR_FRONTEND_VIRTUAL_KEYBOARD_V1_H
