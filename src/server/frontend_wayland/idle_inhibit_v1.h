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

#include "idle-inhibit-unstable-v1_wrapper.h"
#include <mir/scene/idle_hub.h>

#include <memory>

namespace mir
{
class Executor;
namespace frontend
{
auto create_idle_inhibit_manager_v1(
    wl_display* display,
    std::shared_ptr<Executor> wayland_executor,
    std::shared_ptr<scene::IdleHub> idle_hub)
-> std::shared_ptr<wayland::IdleInhibitManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_IDLE_INHIBIT_V1_H_
