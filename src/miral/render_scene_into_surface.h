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

#ifndef MIRAL_SCENE_SURFACE_H
#define MIRAL_SCENE_SURFACE_H

#include <functional>
#include <memory>
#include <mir/geometry/rectangle.h>

namespace mir
{
class Server;
namespace scene
{
class Surface;
}
}

namespace miral
{
/// Renders a part of the scene into a surface.
/// The surface will always render the most up-to-date contents of the scene.
/// The surface is guaranteed to have a size equivalent to the capture area
/// so that it can fit the requested render.
class RenderSceneIntoSurface
{
public:
    RenderSceneIntoSurface();
    ~RenderSceneIntoSurface();

    /// Sets the capture area of the scene.
    RenderSceneIntoSurface& capture_area(mir::geometry::Rectangle const& area);

    /// Set to true if the the cursor should be included in the scene capture.
    RenderSceneIntoSurface& overlay_cursor(bool overlay_cursor);

    /// Provides access to the surface when it is ready.
    RenderSceneIntoSurface& on_surface_ready(std::function<void(std::shared_ptr<mir::scene::Surface> const&)>&& callback);

    void operator()(mir::Server& server);

private:
    class Self;
    std::shared_ptr<Self> self;
};
}

#endif //MIRAL_SCENE_SURFACE_H
