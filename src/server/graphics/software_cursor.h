/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_SOFTWARE_CURSOR_H_
#define MIR_GRAPHICS_SOFTWARE_CURSOR_H_

#include "mir/graphics/cursor.h"
#include "mir_toolkit/client_types.h"
#include "mir/geometry/displacement.h"
#include <mutex>

namespace mir
{
class Executor;
namespace input { class Scene; }
namespace graphics
{
class GraphicBufferAllocator;
class Renderable;

namespace detail
{
class CursorRenderable;
}

class SoftwareCursor : public Cursor
{
public:
    SoftwareCursor(
        std::shared_ptr<GraphicBufferAllocator> const& allocator,
        std::shared_ptr<Executor> const& scene_executor,
        std::shared_ptr<input::Scene> const& scene);
    ~SoftwareCursor();

    void show(CursorImage const& cursor_image) override;
    void hide() override;
    void move_to(geometry::Point position) override;

private:
    std::shared_ptr<detail::CursorRenderable> create_renderable_for(
        CursorImage const& cursor_image, geometry::Point position);

    std::shared_ptr<GraphicBufferAllocator> const allocator;
    std::shared_ptr<input::Scene> const scene;
    MirPixelFormat const format;

    /// If input visualisations are removed from the scene before they are added, the scene throws and Mir crashes
    /// We don't want to call into the scene under lock, so we make these calls on with an executor
    std::shared_ptr<Executor> const scene_executor;

    std::mutex guard;
    std::shared_ptr<detail::CursorRenderable> renderable;
    bool visible;
    geometry::Displacement hotspot;
};

}
}

#endif
