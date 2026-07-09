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
#include "client.h"
#include "weak.h"

#include <cstdint>
#include <map>
#include <memory>
#include <string>

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

/// A helper class to keep track of toplevel IDs. Shared across every client's
/// ext_foreign_toplevel_list_v1 so that a given surface always reports the same
/// stable identifier.
class ForeignToplevelIdentifierMap
{
public:
    auto toplevel_id(std::shared_ptr<scene::Surface> const& surface) -> std::string;
    void forget_toplevel(std::shared_ptr<scene::Surface> const& surface);
    void forget_stale_toplevels();

private:
    std::map<
        std::weak_ptr<scene::Surface>,
        std::string,
        std::owner_less<std::weak_ptr<scene::Surface>>> toplevel_ids;
    uint64_t next_id = 0;
};

/// Used by a client to acquire information about a specific toplevel
class ExtForeignToplevelHandleV1
    : public wayland_rs::ExtForeignToplevelHandleV1
{
public:
    ExtForeignToplevelHandleV1(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::ExtForeignToplevelHandleV1Middleware> instance,
        uint32_t object_id,
        std::shared_ptr<scene::Surface> const& surface);

    static auto from_or_throw(
        wayland_rs::Weak<wayland_rs::ExtForeignToplevelHandleV1> const& handle) -> ExtForeignToplevelHandleV1&;
    auto surface() const -> std::weak_ptr<scene::Surface>;

    /// Sends the .closed event and makes this surface inert
    void should_close();

private:
    std::weak_ptr<scene::Surface> weak_surface;
};


auto create_ext_foreign_toplevel_list_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::ExtForeignToplevelListV1Middleware> instance,
    uint32_t object_id,
    std::shared_ptr<Executor> const& wayland_executor,
    std::shared_ptr<SurfaceStack> const& surface_stack,
    std::shared_ptr<DesktopFileManager> const& desktop_file_manager,
    std::shared_ptr<ForeignToplevelIdentifierMap> const& id_map)
-> std::shared_ptr<wayland_rs::ExtForeignToplevelListV1>;

}
}

#endif // MIR_FRONTEND_FOREIGN_TOPLEVEL_LIST_V1_H
