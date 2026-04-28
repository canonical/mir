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

#ifndef MIR_FRONTEND_VIRTUAL_POINTER_V1_H_
#define MIR_FRONTEND_VIRTUAL_POINTER_V1_H_

#include "wlr_virtual_pointer_unstable_v1.h"

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
struct VirtualPointerV1Ctx;

class VirtualPointerManagerV1
    : public wayland_rs::ZwlrVirtualPointerManagerV1Impl
{
public:
    VirtualPointerManagerV1(
        OutputManager* output_manager,
        std::shared_ptr<input::InputDeviceRegistry> const& device_registry);

    auto create_virtual_pointer(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat, bool has_seat)
        -> std::shared_ptr<wayland_rs::ZwlrVirtualPointerV1Impl> override;
    auto create_virtual_pointer_with_output(wayland_rs::Weak<wayland_rs::WlSeatImpl> const& seat,
        bool has_seat,
        wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output,
        bool has_output) -> std::shared_ptr<wayland_rs::ZwlrVirtualPointerV1Impl> override;

private:
    std::shared_ptr<VirtualPointerV1Ctx> const ctx;
};
}
}

#endif // MIR_FRONTEND_VIRTUAL_POINTER_V1_H_
