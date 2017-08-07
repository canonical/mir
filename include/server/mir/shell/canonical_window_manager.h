/*
 * Copyright © 2015-2016 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_SHELL_CANONICAL_WINDOW_MANAGER_H_
#define MIR_SHELL_CANONICAL_WINDOW_MANAGER_H_

#include "mir/shell/basic_window_manager.h"

#include "mir/geometry/displacement.h"

namespace mir
{
namespace shell
{
class DisplayLayout;

// standard window management algorithm:
//  o Switch apps: tap or click on the corresponding tile
//  o Move window: Alt-leftmousebutton drag (three finger drag)
//  o Resize window: Alt-middle_button drag (two finger drag)
//  o Maximize/restore current window (to display size): Alt-F11
//  o Maximize/restore current window (to display height): Shift-F11
//  o Maximize/restore current window (to display width): Ctrl-F11
//  o client requests to maximize, vertically maximize & restore
class CanonicalWindowManagerPolicy: public WindowManagementPolicy
{
public:

    explicit CanonicalWindowManagerPolicy(
        WindowManagerTools* const tools,
        std::shared_ptr<shell::DisplayLayout> const& display_layout);

    void handle_session_info_updated(SessionInfoMap& session_info, geometry::Rectangles const& displays) override;

    void handle_displays_updated(SessionInfoMap& session_info, geometry::Rectangles const& displays) override;

    auto handle_place_new_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& request_parameters)
    -> scene::SurfaceCreationParameters override;

    void handle_new_surface(std::shared_ptr<scene::Session> const& session, std::shared_ptr<scene::Surface> const& surface) override;

    void handle_modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        SurfaceSpecification const& modifications) override;

    void handle_delete_surface(std::shared_ptr<scene::Session> const& session, std::weak_ptr<scene::Surface> const& surface) override;

    int handle_set_state(std::shared_ptr<scene::Surface> const& surface, MirWindowState value) override;

    bool handle_keyboard_event(MirKeyboardEvent const* event) override;

    bool handle_touch_event(MirTouchEvent const* event) override;

    bool handle_pointer_event(MirPointerEvent const* event) override;

    void handle_raise_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface) override;

    void handle_request_drag_and_drop(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface) override;

private:
    static const int modifier_mask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

    void toggle(MirWindowState state);
    void click(geometry::Point cursor);
    void resize(geometry::Point cursor);
    void drag(geometry::Point cursor);

    // "Mir and Unity: Surfaces, input, and displays (v0.3)" talks about active
    //  *window*,but Mir really only understands surfaces
    void select_active_surface(std::shared_ptr<scene::Surface> const& surface);
    auto active_surface() const -> std::shared_ptr<scene::Surface>;

    bool resize(std::shared_ptr<scene::Surface> const& surface, geometry::Point cursor, geometry::Point old_cursor, geometry::Rectangle bounds);
    bool drag(std::shared_ptr<scene::Surface> surface, geometry::Point to, geometry::Point from, geometry::Rectangle bounds);
    void move_tree(std::shared_ptr<scene::Surface> const& root, geometry::Displacement movement) const;
    void apply_resize(
        std::shared_ptr<mir::scene::Surface> const& surface,
        geometry::Point const& new_pos,
        geometry::Size const& new_size) const;

    WindowManagerTools* const tools;
    std::shared_ptr<DisplayLayout> const display_layout;

    geometry::Rectangle display_area;
    geometry::Point old_cursor{};
    using FullscreenSurfaces = std::set<std::weak_ptr<scene::Surface>, std::owner_less<std::weak_ptr<scene::Surface>>>;

    FullscreenSurfaces fullscreen_surfaces;

    bool resizing = false;
    bool left_resize = false;
    bool top_resize = false;

    std::recursive_mutex active_surface_mutex;
    std::weak_ptr<scene::Surface> active_surface_;
};


#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
using CanonicalWindowManager = WindowManagerConstructor<CanonicalWindowManagerPolicy>;
#pragma GCC diagnostic pop
}
}

#endif /* MIR_SHELL_CANONICAL_WINDOW_MANAGER_H_ */
