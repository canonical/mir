/*
 * Copyright © 2022 Canonical Ltd.
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

#ifndef MIR_FRONTEND_VIRTUAL_POINTER_V1_H_
#define MIR_FRONTEND_VIRTUAL_POINTER_V1_H_

#include "wlr-virtual-pointer-unstable-v1_wrapper.h"

#include <memory>

namespace mir
{
namespace input
{
class InputDeviceRegistry;
}
namespace frontend
{
class OutputManager;

auto create_virtual_pointer_manager_v1(
    wl_display* display,
    OutputManager* output_manager,
    std::shared_ptr<input::InputDeviceRegistry> const& device_registry)
-> std::shared_ptr<wayland::VirtualPointerManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_VIRTUAL_POINTER_V1_H_
