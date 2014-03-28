/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_H_
#define MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_H_

#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/compositor_report.h"
#include "mir/geometry/rectangle.h"
#include <memory>

namespace mir
{
namespace graphics
{
class DisplayBuffer;
}
namespace compositor
{

class Scene;
class Renderer;

class DefaultDisplayBufferCompositor : public DisplayBufferCompositor
{
public:
    DefaultDisplayBufferCompositor(
        graphics::DisplayBuffer& display_buffer,
        std::shared_ptr<Scene> const& scene,
        std::shared_ptr<Renderer> const& renderer,
        std::shared_ptr<CompositorReport> const& report);

    bool composite() override;
    std::weak_ptr<graphics::Cursor> cursor() const override;
    void zoom(float mag) override;
    void on_cursor_movement(geometry::Point const& p);

private:
    void update_viewport();

    graphics::DisplayBuffer& display_buffer;

    std::shared_ptr<Scene> const scene;
    std::shared_ptr<Renderer> const renderer;
    std::shared_ptr<CompositorReport> const report;
    std::shared_ptr<graphics::Cursor> const soft_cursor;

    bool last_pass_rendered_anything;
    geometry::Rectangle viewport;
    geometry::Point cursor_pos;
    float zoom_mag;
};

}
}

#endif /* MIR_COMPOSITOR_DEFAULT_DISPLAY_BUFFER_COMPOSITOR_H_ */
