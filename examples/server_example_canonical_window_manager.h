/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_EXAMPLE_CANONICAL_WINDOW_MANAGER_H_
#define MIR_EXAMPLE_CANONICAL_WINDOW_MANAGER_H_

#include "server_example_basic_window_manager.h"

#include "mir/geometry/displacement.h"

#include <atomic>

///\example server_example_canonical_window_manager.h
// Based on "Mir and Unity: Surfaces, input, and displays (v0.3)"

namespace mir
{
namespace shell { class DisplayLayout; }
namespace examples
{
struct CanonicalSessionInfoCopy
{
    int surfaces{0};
};

struct CanonicalSurfaceInfoCopy
{
    CanonicalSurfaceInfoCopy(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        scene::SurfaceCreationParameters const& params);

    bool can_be_active() const;

    bool can_morph_to(MirSurfaceType new_type) const;
    bool must_have_parent() const;
    bool must_not_have_parent() const;

    void constrain_resize(
        std::shared_ptr<scene::Surface> const& surface,
        geometry::Point& requested_pos,
        geometry::Size& requested_size,
        const bool left_resize,
        const bool top_resize,
        geometry::Rectangle const& bounds) const;

    MirSurfaceType type;
    MirSurfaceState state;
    geometry::Rectangle restore_rect;
    std::weak_ptr<scene::Session> session;
    std::weak_ptr<scene::Surface> parent;
    std::vector<std::weak_ptr<scene::Surface>> children;
    std::shared_ptr<scene::Surface> titlebar;
    frontend::SurfaceId titlebar_id;
    bool is_titlebar = false;
    geometry::Width min_width;
    geometry::Height min_height;
    geometry::Width max_width;
    geometry::Height max_height;
    mir::optional_value<geometry::DeltaX> width_inc;
    mir::optional_value<geometry::DeltaY> height_inc;
    mir::optional_value<shell::SurfaceAspectRatio> min_aspect;
    mir::optional_value<shell::SurfaceAspectRatio> max_aspect;

    void init_titlebar(std::shared_ptr<scene::Surface> const& surface);
    void paint_titlebar(int intensity);

private:

    struct PaintingImpl;

    std::shared_ptr<PaintingImpl> painting_impl;
};

// standard window management algorithm:
//  o Switch apps: tap or click on the corresponding tile
//  o Move window: Alt-leftmousebutton drag
//  o Resize window: Alt-middle_button drag
//  o Maximize/restore current window (to display size): Alt-F11
//  o Maximize/restore current window (to display height): Shift-F11
//  o Maximize/restore current window (to display width): Ctrl-F11
//  o client requests to maximize, vertically maximize & restore
class CanonicalWindowManagerPolicyCopy
{
public:
    using Tools = BasicWindowManagerToolsCopy<CanonicalSessionInfoCopy, CanonicalSurfaceInfoCopy>;
    using CanonicalSessionInfoMap = typename SessionTo<CanonicalSessionInfoCopy>::type;
    using CanonicalSurfaceInfoMap = typename SurfaceTo<CanonicalSurfaceInfoCopy>::type;

    explicit CanonicalWindowManagerPolicyCopy(
        Tools* const tools,
        std::shared_ptr<shell::DisplayLayout> const& display_layout);

    void click(geometry::Point cursor);

    void handle_session_info_updated(CanonicalSessionInfoMap& session_info, geometry::Rectangles const& displays);

    void handle_displays_updated(CanonicalSessionInfoMap& session_info, geometry::Rectangles const& displays);

    void resize(geometry::Point cursor);

    auto handle_place_new_surface(
        std::shared_ptr<scene::Session> const& session,
        scene::SurfaceCreationParameters const& request_parameters)
    -> scene::SurfaceCreationParameters;

    void handle_new_surface(std::shared_ptr<scene::Session> const& session, std::shared_ptr<scene::Surface> const& surface);

    void handle_modify_surface(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<scene::Surface> const& surface,
        shell::SurfaceSpecification const& modifications);

    void handle_delete_surface(std::shared_ptr<scene::Session> const& session, std::weak_ptr<scene::Surface> const& surface);

    int handle_set_state(std::shared_ptr<scene::Surface> const& surface, MirSurfaceState value);

    void drag(geometry::Point cursor);

    bool handle_keyboard_event(MirKeyboardEvent const* event);

    bool handle_touch_event(MirTouchEvent const* event);

    bool handle_pointer_event(MirPointerEvent const* event);

    void generate_decorations_for(
        std::shared_ptr<scene::Session> const& session, std::shared_ptr<scene::Surface> const& surface,
        CanonicalSurfaceInfoMap& surface_map);

private:
    static const int modifier_mask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

    void toggle(MirSurfaceState state);

    // "Mir and Unity: Surfaces, input, and displays (v0.3)" talks about active
    //  *window*,but Mir really only understands surfaces
    void select_active_surface(std::shared_ptr<scene::Surface> const& surface);
    auto active_surface() const -> std::shared_ptr<scene::Surface>;

    bool resize(std::shared_ptr<scene::Surface> const& surface, geometry::Point cursor, geometry::Point old_cursor, geometry::Rectangle bounds);
    bool drag(std::shared_ptr<scene::Surface> surface, geometry::Point to, geometry::Point from, geometry::Rectangle bounds);
    void move_tree(std::shared_ptr<scene::Surface> const& root, geometry::Displacement movement) const;
    void raise_tree(std::shared_ptr<scene::Surface> const& root) const;
    void apply_resize(
        std::shared_ptr<mir::scene::Surface> const& surface,
        std::shared_ptr<mir::scene::Surface> const& titlebar,
        geometry::Point const& new_pos,
        geometry::Size const& new_size) const;

    Tools* const tools;
    std::shared_ptr<shell::DisplayLayout> const display_layout;

    geometry::Rectangle display_area;
    geometry::Point old_cursor{};
    std::weak_ptr<scene::Surface> active_surface_;
};
}
}

#endif /* MIR_EXAMPLE_CANONICAL_WINDOW_MANAGER_H_ */
