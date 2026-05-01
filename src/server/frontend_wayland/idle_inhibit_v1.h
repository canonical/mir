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
#include "weak.h"
#include <mir/scene/idle_hub.h>

#include <memory>

namespace mir
{
class Executor;
namespace frontend
{
struct IdleInhibitV1Ctx;

class IdleInhibitManagerV1 : public wayland_rs::ZwpIdleInhibitManagerV1Impl
{
public:
    IdleInhibitManagerV1(
        std::shared_ptr<Executor> wayland_executor,
        std::shared_ptr<scene::IdleHub> idle_hub);

    auto create_inhibitor(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface)
        -> std::shared_ptr<wayland_rs::ZwpIdleInhibitorV1Impl> override;

private:
    std::shared_ptr<IdleInhibitV1Ctx> const ctx;
};
}
}

#endif // MIR_FRONTEND_IDLE_INHIBIT_V1_H_
