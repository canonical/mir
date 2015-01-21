/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_EXAMPLES_DEMO_COMPOSITOR_H_
#define MIR_EXAMPLES_DEMO_COMPOSITOR_H_

#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/scene.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/renderable.h"
#include "demo_renderer.h"

#include <mutex>
#include <set>

namespace mir
{
namespace compositor
{
class Scene;
class CompositorReport;
}
namespace graphics
{
class DisplayBuffer;
}
namespace examples
{

class DemoCompositor : public compositor::DisplayBufferCompositor
{
public:
    DemoCompositor(
        graphics::DisplayBuffer& display_buffer,
        std::shared_ptr<compositor::CompositorReport> const& report);
    ~DemoCompositor();

    void composite(compositor::SceneElementSequence&& elements) override;

    void zoom(float mag);
    void on_cursor_movement(geometry::Point const& p);

    void set_colour_effect(ColourEffect);

    static void for_each(std::function<void(DemoCompositor&)> f);

private:
    void update_viewport();

    graphics::DisplayBuffer& display_buffer;
    std::shared_ptr<compositor::CompositorReport> const report;
    geometry::Rectangle viewport;
    geometry::Point cursor_pos;
    float zoom_mag;
    DemoRenderer renderer;

    static std::mutex instances_mutex;
    static std::unordered_set<DemoCompositor*> instances;
};

} // namespace examples
} // namespace mir

#endif // MIR_EXAMPLES_DEMO_COMPOSITOR_H_
