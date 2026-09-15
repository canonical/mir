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

#ifndef MIR_FRONTEND_IDLE_INHIBIT_V1_H_
#define MIR_FRONTEND_IDLE_INHIBIT_V1_H_

#include "idle_inhibit_unstable_v1.h"

#include <memory>

namespace mir
{
class Executor;

namespace scene
{
class IdleHub;
}

namespace frontend
{
struct IdleInhibitV1Ctx
{
    std::shared_ptr<mir::Executor> const wayland_executor;
    std::shared_ptr<scene::IdleHub> const idle_hub;
};

class IdleInhibitManagerV1 : public wayland::IdleInhibitManagerV1
{
public:
    IdleInhibitManagerV1(
        std::shared_ptr<wayland::Client> client,
        rust::Box<wayland::IdleInhibitManagerV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<IdleInhibitV1Ctx> ctx);

private:
    auto create_inhibitor(
        wayland::Weak<wayland::Surface> const& surface,
        rust::Box<wayland::IdleInhibitorV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland::IdleInhibitorV1> override;

    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};
}
}

#endif // MIR_FRONTEND_IDLE_INHIBIT_V1_H_
