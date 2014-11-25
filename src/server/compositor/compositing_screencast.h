/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_COMPOSITOR_COMPOSITING_SCREENCAST_H_
#define MIR_COMPOSITOR_COMPOSITING_SCREENCAST_H_

#include "mir/frontend/screencast.h"

#include <unordered_map>
#include <mutex>

namespace mir
{
namespace geometry { struct Rectangle; }
namespace graphics
{
class Display;
class DisplayBuffer;
class GraphicBufferAllocator;
}
namespace compositor
{
class Scene;
namespace detail { struct ScreencastSessionContext; }

class DisplayBufferCompositorFactory;

class CompositingScreencast : public frontend::Screencast
{
public:
    CompositingScreencast(
        std::shared_ptr<Scene> const& scene,
        std::shared_ptr<graphics::Display> const& display,
        std::shared_ptr<graphics::GraphicBufferAllocator> const& buffer_allocator,
        std::shared_ptr<DisplayBufferCompositorFactory> const& db_compositor_factory);

    frontend::ScreencastSessionId create_session(
        geometry::Rectangle const& region,
        geometry::Size const& size,
        MirPixelFormat pixel_format);
    void destroy_session(frontend::ScreencastSessionId id);
    std::shared_ptr<graphics::Buffer> capture(frontend::ScreencastSessionId id);

private:
    frontend::ScreencastSessionId next_available_session_id();
    std::shared_ptr<detail::ScreencastSessionContext>
        create_session_context(geometry::Rectangle const& rect,
            geometry::Size const& size,
            MirPixelFormat pixel_format);

    std::mutex session_mutex;
    std::shared_ptr<Scene> const scene;
    std::shared_ptr<graphics::Display> const display;
    std::shared_ptr<graphics::GraphicBufferAllocator> const buffer_allocator;
    std::shared_ptr<DisplayBufferCompositorFactory> const db_compositor_factory;

    std::unordered_map<frontend::ScreencastSessionId,
                       std::shared_ptr<detail::ScreencastSessionContext>> session_contexts;
};

}
}

#endif /* MIR_FRONTEND_SCREENCAST_H_ */
