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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#ifndef MIR_EXAMPLES_WINDOW_MANAGER_H_
#define MIR_EXAMPLES_WINDOW_MANAGER_H_

#include "mir/input/event_filter.h"
#include "mir/input/scene.h"
#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"
#include "demo_renderer.h"

#include <memory>

namespace mir
{
namespace shell
{
class FocusController;
}
namespace graphics
{
class Display;
}
namespace compositor
{
class Compositor;
}

namespace scene { class Surface; }

namespace examples
{

class WindowManager : public input::EventFilter
{
public: 
    WindowManager();
    ~WindowManager() = default;

    void set_focus_controller(std::shared_ptr<shell::FocusController> const& focus_controller);
    void set_display(std::shared_ptr<graphics::Display> const& display);
    void set_compositor(std::shared_ptr<compositor::Compositor> const& compositor);
    void set_input_scene(std::shared_ptr<input::Scene> const& scene);
    void force_redraw();
    
    bool handle(MirEvent const& event) override;

protected:
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

private:
    std::shared_ptr<shell::FocusController> focus_controller;
    std::shared_ptr<graphics::Display> display;
    std::shared_ptr<compositor::Compositor> compositor;
    std::shared_ptr<input::Scene> input_scene;

    geometry::Point click;
    geometry::Point old_pos;
    geometry::Point old_cursor;
    geometry::Size old_size;
    float old_pinch_diam;
    int max_fingers;  // Maximum number of fingers touched during gesture
    int zoom_exponent = 0;
    ColourEffect colour_effect = none;

    void toggle(ColourEffect);

    enum {left_edge, hmiddle, right_edge} xedge = hmiddle;
    enum {top_edge, vmiddle, bottom_edge} yedge = vmiddle;
    void save_edges(scene::Surface& surf, geometry::Point const& p);
    void resize(scene::Surface& surf, geometry::Point const& cursor) const;
    bool handle_key_event(MirKeyboardEvent const* event);
    bool handle_touch_event(MirTouchEvent const* event);
    bool handle_pointer_event(MirPointerEvent const* event);
};

}
} // namespace mir

#endif // MIR_EXAMPLES_WINDOW_MANAGER_H_
