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

#include "zwlr_foreign_toplevel_handle_v1_binder.h"
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
class Surface;
}
namespace frontend
{
class WlSeat;
class OutputManager;
class SurfaceRegistry;

class LayerShellV1 : public wayland_rs::cpp::ZwlrForeignToplevelHandleV1Binder
{
public:
    LayerShellV1(
        wl_display* display,
        Executor& wayland_executor,
        std::shared_ptr<shell::Shell> const& shell,
        WlSeat& seat,
        OutputManager* output_manager,
        std::shared_ptr<SurfaceRegistry> const& surface_registry);

    static auto get_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>;

    Executor& wayland_executor;
    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;
    std::shared_ptr<SurfaceRegistry> const surface_registry;

    wayland_rs::cpp::ZwlrForeignToplevelHandleV1Handler* create_handler() const override;

private:
    class Instance;
    void bind(wl_resource* new_resource);
};

}
}

#endif // MIR_FRONTEND_LAYER_SHELL_V1_H
