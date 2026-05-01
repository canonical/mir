/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef MIR_FRONTEND_POINTER_CONSTRAINTS_UNSTABLE_V1_H
#define MIR_FRONTEND_POINTER_CONSTRAINTS_UNSTABLE_V1_H

#include "wayland_rs/wayland_rs_cpp/include/pointer_constraints_unstable_v1.h"

#include <memory>

struct wl_display;

namespace mir
{
class Executor;
namespace shell { class Shell; }

namespace frontend
{
class WlSeat;

class PointerConstraintsV1 : public wayland_rs::ZwpPointerConstraintsV1Impl
{
public:
    PointerConstraintsV1(Executor& wayland_executor, std::shared_ptr<shell::Shell> shell);

    auto lock_pointer(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface,
        wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer,
        wayland_rs::Weak<wayland_rs::WlRegionImpl> const& region,
        bool has_region,
        uint32_t lifetime) -> std::shared_ptr<wayland_rs::ZwpLockedPointerV1Impl> override;
    auto confine_pointer(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface,
        wayland_rs::Weak<wayland_rs::WlPointerImpl> const& pointer,
        wayland_rs::Weak<wayland_rs::WlRegionImpl> const& region,
        bool has_region,
        uint32_t lifetime) -> std::shared_ptr<wayland_rs::ZwpConfinedPointerV1Impl> override;

    enum class Lifetime : uint32_t
    {
        oneshot = ZwpPointerConstraintsV1Impl::Lifetime::oneshot,
        persistent = ZwpPointerConstraintsV1Impl::Lifetime::persistent,
    };

private:
    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
};

}
}

#endif  // MIR_FRONTEND_POINTER_CONSTRAINTS_UNSTABLE_V1_H
