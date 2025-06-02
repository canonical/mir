/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_SOFTWARE_CURSOR_H_
#define MIR_GRAPHICS_SOFTWARE_CURSOR_H_

#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
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
        std::shared_ptr<input::Scene> const& scene);
    ~SoftwareCursor();

    void show(std::shared_ptr<CursorImage> const& cursor_image) override;
    void hide() override;
    void move_to(geometry::Point position) override;
    void scale(float) override;
    auto renderable() -> std::shared_ptr<Renderable> override;
    auto needs_compositing() -> bool override;

private:
    std::shared_ptr<detail::CursorRenderable> create_scaled_renderable_for(
        CursorImage const& cursor_image, geometry::Point position);

    std::shared_ptr<GraphicBufferAllocator> const allocator;
    std::shared_ptr<input::Scene> const scene;
    MirPixelFormat const format;

    std::mutex guard;
    std::shared_ptr<detail::CursorRenderable> renderable_;
    bool visible;
    geometry::Displacement hotspot;

    float current_scale{1.0};
    std::shared_ptr<graphics::CursorImage> current_cursor_image;
};

}
}

#endif
