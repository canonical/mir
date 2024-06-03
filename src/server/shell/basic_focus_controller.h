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

#ifndef MIR_BASIC_FOCUS_CONTROLLER_H
#define MIR_BASIC_FOCUS_CONTROLLER_H

#include <mir/shell/focus_controller.h>
#include <mir/scene/observer.h>

namespace mir
{
namespace compositor { class Scene;  }

namespace shell
{
/**
 * Notes:
 *  1. SurfaceStack as an abstract class makes absolutely no sense
 *  2. FocusController should be trivially overridable
 *  3. Focus is universal, meaning that layers don't matter
 *  4. The SurfaceStack is just the SceneGraph, but the FocusController is different
 *  5. Stacking order != focus order
 *
 *  Here's what should happen:
 *  1. SurfaceStack informs us of new/deleted surfaces
 *  2. FocusController updates the focus depending on #1 or outside selection
 *  3. FocusController informs the system that focus has been gained
 *  4. The system does something in response (e.g. raise the surface)
 */

class BasicFocusControllerObserver : public mir::scene::Observer
{
public:
    BasicFocusControllerObserver();
    void surface_added(std::shared_ptr<mir::scene::Surface> const& surface) override;
    void surface_removed(std::shared_ptr<mir::scene::Surface> const& surface) override;
    void surfaces_reordered(mir::scene::SurfaceSet const& affected_surfaces) override;
    void scene_changed() override;
    void surface_exists(std::shared_ptr<mir::scene::Surface> const& surface) override;
    void end_observation() override;
    std::shared_ptr<scene::Surface> focused_surface() const;
    bool is_active() const;
    bool is_focusable(std::shared_ptr<scene::Surface> const&) const;
    void advance(bool within_app);
private:
    std::vector<std::shared_ptr<mir::scene::Surface>> focus_order;
    std::shared_ptr<mir::scene::Surface> raised;
    std::vector<std::shared_ptr<mir::scene::Surface>>::iterator originally_selected_it;
};


/// A focus controller that keeps the most recently focused windows
/// at the top of the queue.
class BasicFocusController : public mir::shell::FocusController
{
public:
    explicit BasicFocusController(std::shared_ptr<compositor::Scene> const&);
    ~BasicFocusController() override;
    void focus_next_session() override;
    void focus_prev_session() override;
    auto focused_session() const -> std::shared_ptr<scene::Session> override;

    void set_popup_grab_tree(std::shared_ptr<scene::Surface> const& surface) override;
    void set_focus_to(
        std::shared_ptr<scene::Session> const& focus_session,
        std::shared_ptr<scene::Surface> const& focus_surface) override;

    // The surface with focus
    std::shared_ptr<scene::Surface> focused_surface() const override;

    auto surface_at(geometry::Point cursor) const -> std::shared_ptr<scene::Surface> override;

    void raise(SurfaceSet const& surfaces) override;
    void swap_z_order(SurfaceSet const& first, SurfaceSet const& second) override;
    void send_to_back(SurfaceSet const& surfaces) override;

private:
    std::shared_ptr<compositor::Scene> scene;
    std::shared_ptr<scene::Observer> observer;
};
}
}


#endif //MIR_BASIC_FOCUS_CONTROLLER_H
