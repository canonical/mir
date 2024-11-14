/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIRAL_DECORATION_H
#define MIRAL_DECORATION_H

#include "mir/geometry/forward.h"
#include "miral/decoration_manager_builder.h"

#include <cstdint>
#include <memory>
#include <functional>

namespace mir
{
namespace shell
{
class Shell;
namespace decoration
{
class Decoration;
struct DeviceEvent;
}
}
namespace scene
{
class Surface;
}
class Server;
}

namespace miral
{
class DecorationAdapter;
class Decoration // Placeholder names
{
public:
    using DecorationBuilder = std::function<std::unique_ptr<DecorationAdapter>(
        std::shared_ptr<mir::shell::Shell> const& shell, std::shared_ptr<mir::scene::Surface> const& surface)>;

    using Pixel = uint32_t;
    using Buffer = Pixel*;
    using DeviceEvent = mir::shell::decoration::DeviceEvent;
    Decoration(
        std::function<void(Buffer, mir::geometry::Size)> render_titlebar,
        std::function<void(Buffer, mir::geometry::Size)> render_left_border,
        std::function<void(Buffer, mir::geometry::Size)> render_right_border,
        std::function<void(Buffer, mir::geometry::Size)> render_bottom_border,

        std::function<void(DeviceEvent& device)> process_enter,
        std::function<void()> process_leave,
        std::function<void()> process_down,
        std::function<void()> process_up,
        std::function<void(DeviceEvent& device)> process_move,
        std::function<void(DeviceEvent& device)> process_drag
    );

    void render();

    static auto create_manager()
        -> std::shared_ptr<miral::DecorationManagerAdapter>;

private:
    struct Self;
    std::unique_ptr<Self> self;
};

void foo(mir::Server& server); // For testing only
}

#endif
