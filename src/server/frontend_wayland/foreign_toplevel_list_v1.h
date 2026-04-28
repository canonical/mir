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

#include "ext_foreign_toplevel_list_v1.h"

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
    : public wayland_rs::ExtForeignToplevelHandleV1Impl,
      public std::enable_shared_from_this<ExtForeignToplevelHandleV1>
{
public:
    ExtForeignToplevelHandleV1(ExtForeignToplevelListV1& manager, std::shared_ptr<scene::Surface> const& surface);

    auto associate(rust::Box<wayland_rs::ExtForeignToplevelHandleV1Ext> instance, uint32_t object_id) -> void override;
    static auto from(wayland_rs::ExtForeignToplevelHandleV1Impl* resource) -> ExtForeignToplevelHandleV1*;
    static auto from_or_throw(wayland_rs::ExtForeignToplevelHandleV1Impl* resource) -> ExtForeignToplevelHandleV1&;
    auto surface() const -> std::shared_ptr<scene::Surface>;

    /// Sends the .closed event and makes this surface inert
    void should_close();

private:
    ExtForeignToplevelListV1& manager;
    std::weak_ptr<scene::Surface> weak_surface;
};

class ForeignToplevelIdentifierMap;

/// Informs a client about toplevels from itself and other clients
/// The Wayland objects it creates for each toplevel can be used to acquire information
/// Useful for task bars and app switchers
class ExtForeignToplevelListV1Global
{
public:
    ExtForeignToplevelListV1Global(
        std::shared_ptr<Executor> const& wayland_executor,
        std::shared_ptr<SurfaceStack> const& surface_stack,
        std::shared_ptr<DesktopFileManager> const& desktop_file_manager);

    std::shared_ptr<Executor> const wayland_executor;
    std::shared_ptr<SurfaceStack> const surface_stack;
    std::shared_ptr<DesktopFileManager> const desktop_file_manager;
    std::shared_ptr<ForeignToplevelIdentifierMap> const id_map;

    auto create() -> std::shared_ptr<wayland_rs::ExtForeignToplevelListV1Impl>;
};

}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_LIST_V1_H
