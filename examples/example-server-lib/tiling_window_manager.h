/*
 * Copyright © 2015-2018 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIRAL_SHELL_TILING_WINDOW_MANAGER_H
#define MIRAL_SHELL_TILING_WINDOW_MANAGER_H

#include "sw_splash.h"

#include <miral/application.h>
#include <miral/window_management_policy.h>
#include <miral/window_manager_tools.h>

#include <mir/geometry/displacement.h>
#include <miral/internal_client.h>


#include <functional>
#include <vector>

using namespace mir::geometry;

// Demonstrate implementing a simple tiling algorithm

// simple tiling algorithm:
//  o Switch apps: tap or click on the corresponding tile
//  o Move window: Alt-leftmousebutton drag (three finger drag)
//  o Resize window: Alt-middle_button drag (four finger drag)
//  o Maximize/restore current window (to tile size): Alt-F11
//  o Maximize/restore current window (to tile height): Shift-F11
//  o Maximize/restore current window (to tile width): Ctrl-F11
//  o client requests to maximize, vertically maximize & restore
class TilingWindowManagerPolicy : public miral::WindowManagementPolicy
{
public:
    explicit TilingWindowManagerPolicy(miral::WindowManagerTools const& tools, std::shared_ptr<SplashSession> const& spinner,
        miral::InternalClientLauncher const& launcher);

    auto place_new_window(
        miral::ApplicationInfo const& app_info,
        miral::WindowSpecification const& request_parameters)
        -> miral::WindowSpecification override;

    void handle_window_ready(miral::WindowInfo& window_info) override;
    void handle_modify_window(miral::WindowInfo& window_info, miral::WindowSpecification const& modifications) override;
    bool handle_keyboard_event(MirKeyboardEvent const* event) override;
    bool handle_touch_event(MirTouchEvent const* event) override;
    bool handle_pointer_event(MirPointerEvent const* event) override;
    void handle_raise_window(miral::WindowInfo& window_info) override;

    void advise_end() override;

    void advise_new_window(miral::WindowInfo const& window_info) override;
    void advise_focus_gained(miral::WindowInfo const& info) override;
    void advise_new_app(miral::ApplicationInfo& application) override;
    void advise_delete_app(miral::ApplicationInfo const& application) override;

    void handle_request_drag_and_drop(miral::WindowInfo& window_info) override;
    void handle_request_move(miral::WindowInfo& window_info, MirInputEvent const* input_event) override;
    void handle_request_resize(miral::WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge) override;

    auto confirm_inherited_move(miral::WindowInfo const& window_info, Displacement movement) -> Rectangle override;

    Rectangle confirm_placement_on_display(const miral::WindowInfo& window_info, MirWindowState new_state,
        Rectangle const& new_placement) override;

private:
    void advise_output_create(miral::Output const& output) override;
    void advise_output_update(miral::Output const& updated, miral::Output const& original) override;
    void advise_output_delete(miral::Output const& output) override;

    static const int modifier_mask =
        mir_input_event_modifier_alt |
        mir_input_event_modifier_shift |
        mir_input_event_modifier_sym |
        mir_input_event_modifier_ctrl |
        mir_input_event_modifier_meta;

    void click(Point cursor);
    void resize(Point cursor);
    void drag(Point cursor);
    void toggle(MirWindowState state);

    miral::Application application_under(Point position);

    void update_tiles(Rectangles const& outputs);
    void update_surfaces(miral::ApplicationInfo& info, Rectangle const& old_tile, Rectangle const& new_tile);

    auto transform_set_state(MirWindowState value) -> MirWindowState;

    static void clip_to_tile(miral::WindowSpecification& parameters, Rectangle const& tile);
    static void resize(miral::Window window, Point cursor, Point old_cursor, Rectangle bounds);

    void constrain_size_and_place(miral::WindowSpecification& mods, miral::Window const& window, Rectangle const& tile) const;

    miral::WindowManagerTools tools;
    std::shared_ptr<SplashSession> spinner;
    miral::InternalClientLauncher const launcher;
    Point old_cursor{};
    Rectangles displays;
    bool dirty_tiles = false;

    class MRUTileList
    {
    public:

        void push(std::shared_ptr<void> const& tile);
        void erase(std::shared_ptr<void> const& tile);

        using Enumerator = std::function<void(std::shared_ptr<void> const& tile)>;

        void enumerate(Enumerator const& enumerator) const;
        auto count() -> size_t { return tiles.size(); }

    private:
        std::vector<std::shared_ptr<void>> tiles;
    };

    MRUTileList tiles;
};

#endif /* MIRAL_SHELL_TILING_WINDOW_MANAGER_H */
