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

#ifndef MIR_FRONTEND_MIR_SHELL_H
#define MIR_FRONTEND_MIR_SHELL_H

#include "mir_shell_unstable_v1.h"

#include <memory>

struct wl_display;

namespace mir
{
namespace frontend
{
class MirShellV1 : public wayland_rs::MirShellV1Impl
{
public:
    MirShellV1(struct wl_resource* resource);

    auto get_regular_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<wayland_rs::MirRegularSurfaceV1Impl> override;
    auto get_floating_regular_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<wayland_rs::MirFloatingRegularSurfaceV1Impl> override;
    auto get_dialog_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface) -> std::shared_ptr<wayland_rs::MirDialogSurfaceV1Impl> override;
    auto get_satellite_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface, wayland_rs::Weak<wayland_rs::MirPositionerV1Impl> const& positioner) -> std::shared_ptr<wayland_rs::MirSatelliteSurfaceV1Impl> override;
    auto create_positioner() -> std::shared_ptr<wayland_rs::MirPositionerV1Impl> override;
};
}
}

#endif //MIR_FRONTEND_MIR_SHELL_H
