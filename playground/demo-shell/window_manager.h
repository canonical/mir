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
#include "mir/geometry/displacement.h"
#include "mir/geometry/size.h"

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
    
    bool handle(MirEvent const& event) override;

protected:
    WindowManager(const WindowManager&) = delete;
    WindowManager& operator=(const WindowManager&) = delete;

private:
    std::shared_ptr<shell::FocusController> focus_controller;
    std::shared_ptr<graphics::Display> display;
    std::shared_ptr<compositor::Compositor> compositor;

    geometry::Point click;
    geometry::Point old_pos;
    geometry::Point old_cursor;
    geometry::Size old_size;
    float old_pinch_diam;
    int max_fingers;  // Maximum number of fingers touched during gesture
};

}
} // namespace mir

#endif // MIR_EXAMPLES_WINDOW_MANAGER_H_
