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

namespace mir
{
namespace frontend
{
auto create_mir_shell_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::MirShellV1Middleware> instance,
    uint32_t object_id) -> std::shared_ptr<wayland_rs::MirShellV1>;
}
}

#endif //MIR_FRONTEND_MIR_SHELL_H
