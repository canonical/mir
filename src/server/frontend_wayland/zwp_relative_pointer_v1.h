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

#ifndef MIR_FRONTEND_ZWP_RELATIVE_POINTER_V1_H
#define MIR_FRONTEND_ZWP_RELATIVE_POINTER_V1_H

#include "relative_pointer_unstable_v1.h"
#include "client.h"

#include <cstdint>
#include <memory>

namespace mir
{
namespace shell { class Shell; }

namespace frontend
{
auto create_relative_pointer_unstable_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::RelativePointerManagerV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<shell::Shell> shell)
-> std::shared_ptr<wayland_rs::RelativePointerManagerV1>;
}
}

#endif  // MIR_FRONTEND_ZWP_RELATIVE_POINTER_V1_H
