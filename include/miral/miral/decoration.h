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

#include <cstdint>
#include <memory>
#include <functional>

namespace mir
{
namespace shell
{
class Shell;
class SurfaceSpecification;
namespace decoration
{
class Decoration;
struct DeviceEvent;
}
}
namespace scene
{
class Surface;
class Session;
}
class Server;
namespace graphics
{
class GraphicBufferAllocator;
}
}

struct WindowState; // TODO: move to miral namespace

namespace miral
{
class DecorationAdapter;
class DecorationManagerAdapter;
class Decoration // Placeholder names
{
public:
    using DecorationBuilder = std::function<std::shared_ptr<DecorationAdapter>(
        std::shared_ptr<mir::shell::Shell> const& shell, std::shared_ptr<mir::scene::Surface> const& surface)>;

    using Pixel = uint32_t;
    using Buffer = Pixel*;
    using DeviceEvent = mir::shell::decoration::DeviceEvent;
    Decoration();

    auto update_state(std::shared_ptr<WindowState> window_state, std::shared_ptr<mir::scene::Surface> window_surface)
        -> mir::shell::SurfaceSpecification;
    auto window_state() -> std::shared_ptr<WindowState>;

    static auto create_manager(mir::Server&)
        -> std::shared_ptr<miral::DecorationManagerAdapter>;

private:
    friend class miral::DecorationAdapter;
    struct Self;
    std::unique_ptr<Self> self;
};
}

#endif
