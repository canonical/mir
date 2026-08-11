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

#ifndef MIR_FRONTEND_ZWP_POINTER_CONSTRAINTS_V1_H
#define MIR_FRONTEND_ZWP_POINTER_CONSTRAINTS_V1_H

#include "pointer_constraints_unstable_v1.h"

#include <memory>

namespace mir
{
class Executor;
namespace shell { class Shell; }

namespace frontend
{
class PointerConstraintsV1 : public wayland::PointerConstraintsV1
{
public:
    PointerConstraintsV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::PointerConstraintsV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<shell::Shell> shell);

private:
    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<shell::Shell> const shell;

    auto lock_pointer(
        wayland::Weak<wayland::Surface> const& surface,
        wayland::Weak<wayland::Pointer> const& pointer,
        std::optional<wayland::Weak<wayland::Region>> const& region,
        uint32_t lifetime,
        rust::Box<wayland::LockedPointerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::LockedPointerV1> override;

    auto confine_pointer(
        wayland::Weak<wayland::Surface> const& surface,
        wayland::Weak<wayland::Pointer> const& pointer,
        std::optional<wayland::Weak<wayland::Region>> const& region,
        uint32_t lifetime,
        rust::Box<wayland::ConfinedPointerV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::ConfinedPointerV1> override;
};

}
}

#endif  // MIR_FRONTEND_ZWP_POINTER_CONSTRAINTS_V1_H
