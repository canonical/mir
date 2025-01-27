/*
 * Copyright © Canonical Ltd.
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
 */

#include "floating_window_manager.h"
#include "decoration_provider.h"
#include "mir_toolkit/common.h"
#include "miral/minimal_window_manager.h"
#include "miral/window_specification.h"

#include <miral/application_info.h>
#include <miral/internal_client.h>
#include <miral/toolkit_event.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>
#include <miral/zone.h>
#include <mir/geometry/rectangle.h>

#include <linux/input.h>
#include <csignal>
#include <algorithm>
#include <ranges>
#include <unordered_set>
#include <functional>

using namespace miral;
using namespace miral::toolkit;

namespace
{
struct PolicyData
{
    bool in_hidden_workspace{false};

    MirWindowState old_state;
};

inline PolicyData& policy_data_for(WindowInfo const& info)
{
    return *std::static_pointer_cast<PolicyData>(info.userdata());
}
}

FloatingWindowManagerPolicy::FloatingWindowManagerPolicy(
    WindowManagerTools const& tools,
    std::shared_ptr<SplashSession> const& spinner,
    miral::InternalClientLauncher const& launcher,
    std::function<void()>& shutdown_hook) :
    FloatingWindowManagerPolicy{tools, spinner, launcher, shutdown_hook, FocusStealing::allow}
{
}

FloatingWindowManagerPolicy::FloatingWindowManagerPolicy(
    WindowManagerTools const& tools,
    std::shared_ptr<SplashSession> const& spinner,
    miral::InternalClientLauncher const& launcher,
    std::function<void()>& shutdown_hook,
    FocusStealing focus_stealing) :
    MinimalWindowManager(tools, focus_stealing),
    spinner{spinner},
    decoration_provider{std::make_unique<DecorationProvider>()},
    focus_stealing{focus_stealing}
{
    launcher.launch(*decoration_provider);
    shutdown_hook = [this] { decoration_provider->stop(); };

    for (auto key : {KEY_F1, KEY_F2, KEY_F3, KEY_F4})
        key_to_workspace[key] = this->tools.create_workspace();

    active_workspace = key_to_workspace[KEY_F1];
}

FloatingWindowManagerPolicy::~FloatingWindowManagerPolicy() = default;

bool FloatingWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    if (MinimalWindowManager::handle_pointer_event(event))
        return true;

    auto const action = mir_pointer_event_action(event);
    auto const modifiers = mir_pointer_event_modifiers(event) & modifier_mask;
    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    bool consumes_event = false;

    if (action == mir_pointer_action_button_down)
    {
        if (auto const window = tools.window_at(cursor))
            tools.select_active_window(window);

        if (auto const window = tools.active_window())
        {
            if (mir_pointer_event_button_state(event, mir_pointer_button_tertiary))
            {
                if (modifiers == mir_input_event_modifier_alt)
                {
                    Rectangle const old_pos{window.top_left(), window.size()};

                    auto anchor = old_pos.bottom_right();
                    auto edge = mir_resize_edge_northwest;

                    struct Corner { Point point; MirResizeEdge edge; };

                    for (auto const& corner : {
                        Corner{old_pos.top_right(), mir_resize_edge_southwest},
                        Corner{old_pos.bottom_left(), mir_resize_edge_northeast},
                        Corner{old_pos.top_left, mir_resize_edge_southeast}})
                    {
                        if ((cursor - anchor).length_squared() <
                            (cursor - corner.point).length_squared())
                        {
                            anchor = corner.point;
                            edge = corner.edge;
                        }
                    }

                    begin_pointer_resize(tools.info_for(window), mir_pointer_event_input_event(event), edge);
                    consumes_event = true;
                }
            }
        }
    }

    return consumes_event;
}

bool FloatingWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event)
{
    auto const count = mir_touch_event_point_count(event);

    if (MinimalWindowManager::handle_touch_event(event) || count != 3)
    {
        pinching = false;
        return false;
    }

    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_event_action(event, i))
        {
        case mir_touch_action_up:
        case mir_touch_action_down:
            pinching = false;
            return false;

        default:
            continue;
        }
    }

    int touch_pinch_top = std::numeric_limits<int>::max();
    int touch_pinch_left = std::numeric_limits<int>::max();
    int touch_pinch_width = 0;
    int touch_pinch_height = 0;

    for (auto i = 0U; i != count; ++i)
    {
        for (auto j = 0U; j != i; ++j)
        {
            int dx = mir_touch_event_axis_value(event, i, mir_touch_axis_x) -
                     mir_touch_event_axis_value(event, j, mir_touch_axis_x);

            int dy = mir_touch_event_axis_value(event, i, mir_touch_axis_y) -
                     mir_touch_event_axis_value(event, j, mir_touch_axis_y);

            if (touch_pinch_width < dx)
                touch_pinch_width = dx;

            if (touch_pinch_height < dy)
                touch_pinch_height = dy;
        }

        int const x = mir_touch_event_axis_value(event, i, mir_touch_axis_x);

        int const y = mir_touch_event_axis_value(event, i, mir_touch_axis_y);

        if (touch_pinch_top > y)
            touch_pinch_top = y;

        if (touch_pinch_left > x)
            touch_pinch_left = x;
    }

    if (auto window = tools.active_window())
    {
        auto const old_size = window.size();
        auto const delta_width = DeltaX{touch_pinch_width - old_touch_pinch_width};
        auto const delta_height = DeltaY{touch_pinch_height - old_touch_pinch_height};

        auto new_width = std::max(old_size.width + delta_width, Width{5});
        auto new_height = std::max(old_size.height + delta_height, Height{5});
        Displacement movement{
            DeltaX{touch_pinch_left - old_touch_pinch_left},
            DeltaY{touch_pinch_top  - old_touch_pinch_top}};

        auto& window_info = tools.info_for(window);
        keep_window_within_constraints(window_info, movement, new_width, new_height);

        auto new_pos = window.top_left() + movement;
        Size new_size{new_width, new_height};

        {   // Workaround for lp:1627697
            auto now = std::chrono::steady_clock::now();
            if (pinching && now < last_resize+std::chrono::milliseconds(20))
                return true;

            last_resize = now;
        }

        if (pinching)
        {
            WindowSpecification modifications;
            modifications.top_left() = new_pos;
            modifications.size() = new_size;
            tools.modify_window(window_info, modifications);
        }
        else
        {
            pinching = true;
        }

        old_touch_pinch_top = touch_pinch_top;
        old_touch_pinch_left = touch_pinch_left;
        old_touch_pinch_width = touch_pinch_width;
        old_touch_pinch_height = touch_pinch_height;
    }

    return true;
}

void FloatingWindowManagerPolicy::advise_new_window(WindowInfo const& window_info)
{
    MinimalWindowManager::advise_new_window(window_info);

    auto const parent = window_info.parent();

    if (!parent)
        tools.add_tree_to_workspace(window_info.window(), active_workspace);
    else
    {
        if (policy_data_for(tools.info_for(parent)).in_hidden_workspace)
            apply_workspace_hidden_to(window_info.window());
    }
}

void FloatingWindowManagerPolicy::handle_window_ready(WindowInfo& window_info)
{
    MinimalWindowManager::handle_window_ready(window_info);
    keep_spinner_on_top();
}

void FloatingWindowManagerPolicy::advise_focus_gained(WindowInfo const& info)
{
    MinimalWindowManager::advise_focus_gained(info);
    keep_spinner_on_top();
}

void FloatingWindowManagerPolicy::keep_spinner_on_top()
{
    // Frig to force the spinner to the top
    if (auto const spinner_session = spinner->session())
    {
        auto const& spinner_info = tools.info_for(spinner_session);

        for (auto const& window : spinner_info.windows())
            tools.raise_tree(window);
    }
}

bool FloatingWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    if (MinimalWindowManager::handle_keyboard_event(event))
        return true;

    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

    // Switch workspaces
    if (action == mir_keyboard_action_down &&
        modifiers == (mir_input_event_modifier_alt | mir_input_event_modifier_meta))
    {
        auto const found = key_to_workspace.find(scan_code);

        if (found != key_to_workspace.end())
        {
            switch_workspace_to(found->second);
            return true;
        }
    }

    // Switch workspace taking the active window
    if (action == mir_keyboard_action_down &&
        modifiers == (mir_input_event_modifier_ctrl | mir_input_event_modifier_meta))
    {
        auto const found = key_to_workspace.find(scan_code);

        if (found != key_to_workspace.end())
        {
            switch_workspace_to(found->second, tools.active_window());
            return true;
        }
    }

    if (action == mir_keyboard_action_down && scan_code == KEY_F11)
    {
        switch (modifiers)
        {
        case mir_input_event_modifier_alt:
            toggle(mir_window_state_maximized);
            return true;

        case mir_input_event_modifier_shift:
            toggle(mir_window_state_vertmaximized);
            return true;

        case mir_input_event_modifier_ctrl:
            toggle(mir_window_state_horizmaximized);
            return true;

        case mir_input_event_modifier_meta:
            toggle(mir_window_state_fullscreen);
            return true;

        default:
            break;
        }
    }
    else if (action == mir_keyboard_action_down && scan_code == KEY_F4 &&
        modifiers == (mir_input_event_modifier_alt|mir_input_event_modifier_shift))
    {
        if (auto const& window = tools.active_window())
            kill(window.application(), SIGTERM);
        return true;
    }
    else if (action == mir_keyboard_action_down &&
             modifiers == (mir_input_event_modifier_ctrl|mir_input_event_modifier_meta))
    {
        if (auto active_window = tools.active_window())
        {
            auto active_zone = tools.active_application_zone().extents();
            auto& window_info = tools.info_for(active_window);
            WindowSpecification modifications;

            switch (scan_code)
            {
            case KEY_LEFT:
                modifications.state() = mir_window_state_vertmaximized;
                tools.place_and_size_for_state(modifications, window_info);
                modifications.top_left() = active_zone.top_left;
                break;

            case KEY_RIGHT:
            {
                modifications.state() = mir_window_state_vertmaximized;
                tools.place_and_size_for_state(modifications, window_info);

                auto const new_width =
                    (modifications.size().is_set() ? modifications.size().value() : active_window.size()).width;

                modifications.top_left() = active_zone.top_right() - Displacement{as_delta(new_width), 0};
                break;
            }

            case KEY_UP:
                modifications.state() = mir_window_state_horizmaximized;
                tools.place_and_size_for_state(modifications, window_info);
                modifications.top_left() = active_zone.top_left;
                break;

            case KEY_DOWN:
                modifications.state() = mir_window_state_horizmaximized;
                tools.place_and_size_for_state(modifications, window_info);
                modifications.top_left() =
                    active_zone.bottom_right() - as_displacement(
                    modifications.size().is_set() ? modifications.size().value() : active_window.size());
                break;

            default:
                return false;
            }

            tools.modify_window(window_info, modifications);
            return true;
        }
    }

    return false;
}

void FloatingWindowManagerPolicy::toggle(MirWindowState state)
{
    if (auto const window = tools.active_window())
    {
        auto& info = tools.info_for(window);

        WindowSpecification modifications;

        modifications.state() = (info.state() == state) ? mir_window_state_restored : state;
        tools.place_and_size_for_state(modifications, info);
        tools.modify_window(info, modifications);
    }
}

void FloatingWindowManagerPolicy::keep_window_within_constraints(
    WindowInfo const& window_info, Displacement& movement, Width& new_width, Height& new_height) const
{
    switch (window_info.state())
    {
    case mir_window_state_maximized:
    case mir_window_state_fullscreen:
        new_width = window_info.window().size().width;
        new_height = window_info.window().size().height;
        movement = {0, 0};
        break;

    case mir_window_state_vertmaximized:
        new_height = window_info.window().size().height;
        movement.dy = DeltaY{0};
        break;

    case mir_window_state_horizmaximized:
        new_width = window_info.window().size().width;
        movement.dx - DeltaX{0};
        break;

    default:;
    }

    auto const min_width  = std::max(window_info.min_width(), Width{5});
    auto const min_height = std::max(window_info.min_height(), Height{5});

    if (new_width < min_width)
    {
        new_width = min_width;
        if (movement.dx > DeltaX{0})
            movement.dx = DeltaX{0};
    }

    if (new_height < min_height)
    {
        new_height = min_height;
        if (movement.dy > DeltaY{0})
            movement.dy = DeltaY{0};
    }

    auto const max_width  = window_info.max_width();
    auto const max_height = window_info.max_height();

    if (new_width > max_width)
    {
        new_width = max_width;
        if (movement.dx < DeltaX{0})
            movement.dx = DeltaX{0};
    }

    if (new_height > max_height)
    {
        new_height = max_height;
        if (movement.dy < DeltaY{0})
            movement.dy = DeltaY{0};
    }
}

WindowSpecification FloatingWindowManagerPolicy::place_new_window(
    ApplicationInfo const& app_info, WindowSpecification const& request_parameters)
{
    auto parameters = MinimalWindowManager::place_new_window(app_info, request_parameters);

    if(focus_stealing == FocusStealing::prevent)
        try_place_new_window_and_account_for_occlusion(parameters);

    if (app_info.application() == decoration_provider->session())
    {
        parameters.type() = mir_window_type_decoration;
        parameters.depth_layer() = mir_depth_layer_background;
    }

    parameters.userdata() = std::make_shared<PolicyData>();
    return parameters;
}

void FloatingWindowManagerPolicy::advise_adding_to_workspace(
    std::shared_ptr<Workspace> const& workspace, std::vector<Window> const& windows)
{
    if (windows.empty())
        return;

    for (auto const& window : windows)
    {
        if (workspace == active_workspace)
        {
            apply_workspace_visible_to(window);
        }
        else
        {
            apply_workspace_hidden_to(window);
        }
    }
}

void FloatingWindowManagerPolicy::switch_workspace_to(
    std::shared_ptr<Workspace> const& workspace,
    Window const& window)
{
    if (workspace == active_workspace)
        return;

    auto const old_active = active_workspace;
    active_workspace = workspace;

    auto const old_active_window = tools.active_window();

    if (!old_active_window)
    {
        // If there's no active window, the first shown grabs focus: get the right one
        if (auto const ww = workspace_to_active[workspace])
        {
            tools.for_each_workspace_containing(ww, [&](std::shared_ptr<miral::Workspace> const& ws)
                {
                    if (ws == workspace)
                    {
                        apply_workspace_visible_to(ww);
                    }
                });
        }
    }

    tools.remove_tree_from_workspace(window, old_active);
    tools.add_tree_to_workspace(window, active_workspace);

    tools.for_each_window_in_workspace(active_workspace, [&](Window const& window)
        {
            if (decoration_provider->is_decoration(window))
                return; // decorations are taken care of automatically

            apply_workspace_visible_to(window);
        });

    bool hide_old_active = false;
    tools.for_each_window_in_workspace(old_active, [&](Window const& window)
        {
            if (decoration_provider->is_decoration(window))
                return; // decorations are taken care of automatically

            if (window == old_active_window)
            {
                // If we hide the active window focus will shift: do that last
                hide_old_active = true;
                return;
            }

            apply_workspace_hidden_to(window);
        });

    if (hide_old_active)
    {
        apply_workspace_hidden_to(old_active_window);

        // Remember the old active_window when we switch away
        workspace_to_active[old_active] = old_active_window;
    }
}

void FloatingWindowManagerPolicy::apply_workspace_hidden_to(Window const& window)
{
    auto const& window_info = tools.info_for(window);
    auto& pdata = policy_data_for(window_info);
    if (!pdata.in_hidden_workspace)
    {
        pdata.in_hidden_workspace = true;
        pdata.old_state = window_info.state();

        WindowSpecification modifications;
        modifications.state() = mir_window_state_hidden;
        tools.place_and_size_for_state(modifications, window_info);
        tools.modify_window(window_info.window(), modifications);
    }
}

void FloatingWindowManagerPolicy::apply_workspace_visible_to(Window const& window)
{
    auto const& window_info = tools.info_for(window);
    auto& pdata = policy_data_for(window_info);
    if (pdata.in_hidden_workspace)
    {
        pdata.in_hidden_workspace = false;
        WindowSpecification modifications;
        modifications.state() = pdata.old_state;
        tools.place_and_size_for_state(modifications, window_info);
        tools.modify_window(window_info.window(), modifications);
    }
}

void FloatingWindowManagerPolicy::handle_modify_window(WindowInfo& window_info, WindowSpecification const& modifications)
{
    auto mods = modifications;

    auto& pdata = policy_data_for(window_info);

    if (pdata.in_hidden_workspace && mods.state().is_set())
        pdata.old_state = mods.state().consume();

    MinimalWindowManager::handle_modify_window(window_info, mods);
}

void FloatingWindowManagerPolicy::try_place_new_window_and_account_for_occlusion(WindowSpecification& parameters)
{
    namespace geometry = mir::geometry;
    if (parameters.state() == mir_window_state_restored)
    {
        std::vector<geometry::Rectangle> window_rects;
        tools.for_each_window_in_workspace(
            active_workspace,
            [this, &window_rects](Window const& window)
            {
                auto const& info = tools.info_for(window);

                // Skip invisible windows
                if (!info.is_visible() || info.depth_layer() != mir_depth_layer_application)
                    return;

                window_rects.emplace_back(window.top_left(), window.size());
            }
        );

        // Worst case scenario: you try taking a hole out of the middle of
        // a rect, results in 4 rectangles. Every other case is a special
        // case of this one where the width or height of the resulting
        // rects are < 0.
        auto const subtract_rectangles = [](geometry::Rectangle rect,
                                            geometry::Rectangle hole) -> std::vector<geometry::Rectangle>
        {
            if (!rect.overlaps(hole))
                return {rect};

            if (hole.size == geometry::Size{0, 0})
                return {};

            auto const rect_a =
                geometry::Rectangle{rect.top_left, geometry::Size{rect.size.width, (hole.top() - rect.top()).as_int()}};

            auto const rect_b = geometry::Rectangle{
                geometry::Point{rect.left(), rect.top()},
                geometry::Size{(hole.left() - rect.left()).as_int(), rect.size.height}};

            auto const rect_c = geometry::Rectangle{
                geometry::Point{hole.right(), rect.top()},
                geometry::Size{(rect.right() - hole.right()).as_int(), rect.size.height}};

            auto const rect_d = geometry::Rectangle{
                geometry::Point{rect.left(), hole.bottom()},
                geometry::Size{rect.size.width, (rect.bottom() - hole.bottom()).as_int()}};

            auto const valid_rect = [](auto const rect)
            {
                return rect.size.width.as_int() > 0 && rect.size.height.as_int() > 0;
            };
            // Funny, you can't have valid_rects be const and use `.cbegin`
            auto valid_rects = std::vector{rect_a, rect_b, rect_c, rect_d} | std::ranges::views::filter(valid_rect);

            return std::vector(valid_rects.cbegin(), valid_rects.cend());
        };

        using RectangleSet = std::unordered_set<
                    geometry::Rectangle,
                    decltype([](geometry::Rectangle const& s) {
                        return std::hash<int>{}(s.size.height.as_int() * 31 + s.size.width.as_int());
                    })>;

        auto const get_visible_rects =
            [subtract_rectangles](geometry::Rectangle initial_test_rect, std::vector<geometry::Rectangle> window_rects)
        {
            if (window_rects.empty())
                return RectangleSet{initial_test_rect};

            // Starting with rectangle we're trying to place, repeatedly
            // subtract all visible rectangles from it, accumulating
            // subtractions as we go.
            //
            // In the end, if no rectangles remain, then the window is
            // fully occluded. Otherwise, whatever rectangles remain are
            // the visible part.
            RectangleSet test_rects{initial_test_rect};
            for (auto const& window_rect : window_rects)
            {
                RectangleSet iter_output_rects;
                for (auto const& test_rect : test_rects)
                {
                    auto const window_output_rects = subtract_rectangles(test_rect, window_rect);
                    iter_output_rects.insert(window_output_rects.begin(), window_output_rects.end());
                }

                test_rects = std::move(iter_output_rects);

                if (test_rects.empty())
                    return test_rects;
            }

            return test_rects;
        };

        auto const visible_area_large_enough = [](RectangleSet const& visible_rects)
        {
            auto constexpr min_visible_width = 50;
            auto constexpr min_visible_height = 50;

            return std::ranges::any_of(
                visible_rects,
                [min_visible_width, min_visible_height](auto const& rect)
                {
                    return rect.size.width.as_int() >= min_visible_width &&
                           rect.size.height.as_int() >= min_visible_height;
                });

        };

        // Try place the window around the suggested position
        auto const max_iterations{16};
        for (auto multiplier = 1; multiplier < max_iterations; multiplier++)
        {
            auto const offset{48 * multiplier};
            std::array const positions{
                parameters.top_left().value(),
                parameters.top_left().value() + Displacement(offset, offset),
                parameters.top_left().value() + Displacement(-offset, offset),
                parameters.top_left().value() + Displacement(-offset, -offset),
                parameters.top_left().value() + Displacement(offset, -offset)};

            // Make sure the active output contains any test point
            auto const all_positions_invalid = std::ranges::all_of(
                positions,
                [this](auto const position)
                {
                    return !tools.active_output().contains(position);
                });

            if (all_positions_invalid)
                return;

            for (auto const& position : positions)
            {
                auto const test_rect = geometry::Rectangle{position, parameters.size().value_or({})};

                // Make sure the test rectangle overlaps with the current output
                if (!tools.active_output().overlaps(test_rect))
                    continue;

                auto const visible_rects = get_visible_rects(test_rect, window_rects);
                if (!visible_rects.empty() && visible_area_large_enough(visible_rects))
                {
                    parameters.top_left().value() = position;
                    return;
                }
            }
        }

        // If we can't find any valid offset position, we use the original
        // one, possibly occluding / being occluded by another window.
    }
}
