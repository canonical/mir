/*
 * Copyright © 2018 Canonical Ltd.
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
 *
 * Authored by: William Wold <william.wold@canonical.com>
 */

#ifndef MIR_FRONTEND_LAYER_SHELL_V1_H
#define MIR_FRONTEND_LAYER_SHELL_V1_H

#include "wlr-layer-shell-unstable-v1_wrapper.h"

namespace mir
{
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

class LayerShellV1 : public wayland::LayerShellV1::Global
{
public:
    LayerShellV1(
        wl_display* display,
        std::shared_ptr<shell::Shell> shell,
        WlSeat& seat,
        OutputManager* output_manager);

    static auto get_window(wl_resource* surface) -> std::shared_ptr<scene::Surface>;

    std::shared_ptr<shell::Shell> const shell;
    WlSeat& seat;
    OutputManager* const output_manager;

private:
    class Instance;
    void bind(wl_resource* new_resource);
};

}
}

#endif // MIR_FRONTEND_LAYER_SHELL_V1_H
