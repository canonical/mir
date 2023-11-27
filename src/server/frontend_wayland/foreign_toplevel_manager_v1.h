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

#ifndef MIR_FRONTEND_FOREIGN_TOPLEVEL_MANAGER_V1_H
#define MIR_FRONTEND_FOREIGN_TOPLEVEL_MANAGER_V1_H

#include "wlr-foreign-toplevel-management-unstable-v1_wrapper.h"

#include <memory>

namespace mir
{
class Executor;
class MainLoop;
namespace shell
{
class Shell;
}
namespace frontend
{
class SurfaceStack;

auto create_foreign_toplevel_manager_v1(
    wl_display* display,
    std::shared_ptr<shell::Shell> const& shell,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<MainLoop> const& main_loop)
-> std::shared_ptr<wayland::ForeignToplevelManagerV1::Global>;
}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_MANAGER_V1_H
