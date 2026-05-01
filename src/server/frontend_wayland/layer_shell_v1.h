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

#include <wayland-server.h>
#include <wlcs/display_server.h>

#include "wlr_layer_shell_unstable_v1.h"
#include "client.h"

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
namespace frontend
{
class WlSeatGlobal;
class OutputManager;
class SurfaceRegistry;

class LayerShellV1 : public wayland_rs::ZwlrLayerShellV1Impl
{
public:
    LayerShellV1(
        std::shared_ptr<wayland_rs::Client> const& client,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeatGlobal& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    auto get_layer_surface(wayland_rs::Weak<wayland_rs::WlSurfaceImpl> const& surface, wayland_rs::Weak<wayland_rs::WlOutputImpl> const& output, bool has_output, uint32_t layer, rust::String namespace_)
        -> std::shared_ptr<wayland_rs::ZwlrLayerSurfaceV1Impl> override;
    static auto get_window(wayland_rs::ZwlrLayerSurfaceV1Impl* surface) -> std::shared_ptr<scene::Surface>;

    std::shared_ptr<wayland_rs::Client> client;
    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeatGlobal& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;

private:
    class Instance;
    void bind(wl_resource* new_resource);
};

}
}

#endif // MIR_FRONTEND_LAYER_SHELL_V1_H
