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

#ifndef MIR_FRONTEND_FOREIGN_TOPLEVEL_LIST_V1_H
#define MIR_FRONTEND_FOREIGN_TOPLEVEL_LIST_V1_H

#include "ext-foreign-toplevel-list-v1_wrapper.h"

#include <memory>

namespace mir
{
class Executor;
namespace scene
{
class Surface;
}
namespace shell
{
class Shell;
}
namespace frontend
{
class DesktopFileManager;
class SurfaceStack;
class ExtForeignToplevelListV1;

/// Used by a client to aquire information about a specific toplevel
class ExtForeignToplevelHandleV1
    : public wayland::ExtForeignToplevelHandleV1
{
public:
    ExtForeignToplevelHandleV1(ExtForeignToplevelListV1 const& manager, std::shared_ptr<scene::Surface> const& surface);

    static auto from(wl_resource* resource) -> ExtForeignToplevelHandleV1*;
    static auto from_or_throw(wl_resource* resource) -> ExtForeignToplevelHandleV1&;
    auto surface() const -> std::shared_ptr<scene::Surface>;

    /// Sends the .closed event and makes this surface inert
    void should_close();

private:
    std::weak_ptr<scene::Surface> weak_surface;
};


auto create_ext_foreign_toplevel_list_v1(
    wl_display* display,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager)
-> std::shared_ptr<wayland::ExtForeignToplevelListV1::Global>;

}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_LIST_V1_H
