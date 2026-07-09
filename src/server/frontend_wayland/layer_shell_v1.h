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

#ifndef MIR_FRONTEND_LAYER_SHELL_V1_H
#define MIR_FRONTEND_LAYER_SHELL_V1_H

#include "wlr_layer_shell_unstable_v1.h"

#include <memory>
#include <optional>

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
class Surface;
}
namespace wayland_rs
{
class Client;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class SurfaceRegistry;

class LayerShellV1 : public wayland_rs::LayerShellV1
{
public:
    LayerShellV1(
        std::shared_ptr<wayland_rs::Client> client,
        rust::Box<wayland_rs::LayerShellV1Middleware> instance,
        uint32_t object_id,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    static auto get_window(
        wayland_rs::Weak<wayland_rs::LayerSurfaceV1> const& surface) -> std::shared_ptr<scene::Surface>;

    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;

private:
    using wayland_rs::LayerShellV1::get_layer_surface;
    auto get_layer_surface(
        wayland_rs::Weak<wayland_rs::Surface> const& surface,
        std::optional<wayland_rs::Weak<wayland_rs::Output>> const& output,
        uint32_t layer,
        rust::String namespace_,
        rust::Box<wayland_rs::LayerSurfaceV1Middleware> child_instance,
        uint32_t child_object_id) -> std::shared_ptr<wayland_rs::LayerSurfaceV1> override;
};

auto create_layer_shell_v1(
    std::shared_ptr<wayland_rs::Client> client,
    rust::Box<wayland_rs::LayerShellV1Middleware> instance,
    uint32_t object_id,
    Executor& wayland_executor,
    std::shared_ptr<shell::Shell> const& shell,
    WlSeat& seat,
    OutputManager* output_manager,
    std::shared_ptr<SurfaceRegistry> const& surface_registry)
-> std::shared_ptr<wayland_rs::LayerShellV1>;

}
}

#endif // MIR_FRONTEND_LAYER_SHELL_V1_H
