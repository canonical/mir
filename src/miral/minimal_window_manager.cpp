/*
 * Copyright © Canonical Ltd.
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
 */

#include "application_selector.h"
#include "mir/geometry/forward.h"

#include <miral/minimal_window_manager.h>
#include <miral/toolkit_event.h>
#include <miral/application_info.h>

#include <linux/input.h>
#include <gmpxx.h>

#include <algorithm>
#include <ranges>
#include <unordered_set>

using namespace miral::toolkit;

namespace
{
unsigned int const shift_states =
    mir_input_event_modifier_alt |
    mir_input_event_modifier_shift |
    mir_input_event_modifier_sym |
    mir_input_event_modifier_ctrl |
    mir_input_event_modifier_meta;

enum class Gesture
{
    none,
    pointer_moving,
    pointer_resizing,
    touch_moving,
    touch_resizing,
};

auto pointer_position(MirPointerEvent const* event) -> mir::geometry::Point
{
    return {
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};
}

auto touch_center(MirTouchEvent const* event) -> mir::geometry::Point
{
    auto const count = mir_touch_event_point_count(event);

    long total_x = 0;
    long total_y = 0;

    for (auto i = 0U; i != count; ++i)
    {
        total_x += mir_touch_event_axis_value(event, i, mir_touch_axis_x);
        total_y += mir_touch_event_axis_value(event, i, mir_touch_axis_y);
    }

    return {total_x/count, total_y/count};
}
}

struct miral::MinimalWindowManager::Impl
{
    Impl(WindowManagerTools const& tools, MirInputEventModifier pointer_drag_modifier, FocusStealing focus_stealing) :
        tools{tools},
        application_selector(tools),
        pointer_drag_modifier{pointer_drag_modifier},
        focus_stealing{focus_stealing}
    {
    }

    WindowManagerTools tools;

    Gesture gesture = Gesture::none;
    MirPointerButton pointer_gesture_button;
    miral::Window gesture_window;
    unsigned gesture_shift_keys = 0;
    MirResizeEdge resize_edge = mir_resize_edge_none;
    Point resize_top_left;
    Size resize_size;
    Point old_cursor{};
    Point old_touch{};

    /// Responsible for managing Alt + Tab behavior.
    ApplicationSelector application_selector;

    bool prepare_for_gesture(WindowInfo& window_info, Point input_pos, Gesture gesture);

    bool begin_pointer_gesture(
        WindowInfo& window_info, MirInputEvent const* input_event, Gesture gesture, MirResizeEdge edge);

    bool begin_touch_gesture(
        WindowInfo& window_info, MirInputEvent const* input_event, Gesture gesture, MirResizeEdge edge);

    bool handle_pointer_event(MirPointerEvent const* event);

    bool handle_touch_event(MirTouchEvent const* event);

    void apply_resize_by(Displacement movement);

    // Returns true if the window should NOT be in focus
    // Relevant for focus stealing prevention where windows are focused on
    // launch in specific circumstances.
    bool should_prevent_focus(WindowInfo const& info);

    bool advise_new_window(WindowInfo const& info);

    void try_place_new_window_and_account_for_occlusion(miral::WindowSpecification&);

    MirInputEventModifier const pointer_drag_modifier;
    FocusStealing const focus_stealing;
};

miral::MinimalWindowManager::MinimalWindowManager(WindowManagerTools const& tools) :
    MinimalWindowManager{tools, mir_input_event_modifier_alt}
{
}

miral::MinimalWindowManager::MinimalWindowManager(WindowManagerTools const& tools, FocusStealing focus_stealing) :
    tools{tools},
    self{new Impl{tools, mir_input_event_modifier_alt, focus_stealing}}
{
}

miral::MinimalWindowManager::MinimalWindowManager(WindowManagerTools const& tools, MirInputEventModifier pointer_drag_modifier):
    tools{tools},
    self{new Impl{tools, pointer_drag_modifier, FocusStealing::allow}}
{
}

miral::MinimalWindowManager::MinimalWindowManager(WindowManagerTools const& tools, MirInputEventModifier pointer_drag_modifier, FocusStealing focus_stealing):
    tools{tools},
    self{new Impl{tools, pointer_drag_modifier, focus_stealing}}
{
}

miral::MinimalWindowManager::~MinimalWindowManager()
{
    delete self;
}

auto miral::MinimalWindowManager::place_new_window(
    ApplicationInfo const& /*app_info*/, WindowSpecification const& requested_specification)
    -> WindowSpecification
{
    auto spec_copy = requested_specification;
    self->try_place_new_window_and_account_for_occlusion(spec_copy);
    return spec_copy;
}

void miral::MinimalWindowManager::handle_window_ready(WindowInfo& window_info)
{
    if (!self->should_prevent_focus(window_info) && window_info.can_be_active())
    {
        tools.select_active_window(window_info.window());
    }
}

void miral::MinimalWindowManager::handle_modify_window(
    WindowInfo& window_info, miral::WindowSpecification const& modifications)
{
    tools.modify_window(window_info, modifications);
}

void miral::MinimalWindowManager::handle_raise_window(WindowInfo& window_info)
{
    tools.select_active_window(window_info.window());
}

auto miral::MinimalWindowManager::confirm_placement_on_display(
    WindowInfo const& /*window_info*/, MirWindowState /*new_state*/, Rectangle const& new_placement)
    -> Rectangle
{
    return new_placement;
}

bool miral::MinimalWindowManager::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const shift_state = mir_keyboard_event_modifiers(event) & shift_states;

    if (action == mir_keyboard_action_down && shift_state == mir_input_event_modifier_alt)
    {
        switch (mir_keyboard_event_scan_code(event))
        {
        case KEY_F4:
            tools.ask_client_to_close(tools.active_window());
            return true;

        case KEY_TAB:
            self->application_selector.next(false);
            return true;

        case KEY_GRAVE:
            self->application_selector.next(true);
            return true;

        default:;
        }
    }

    if (action == mir_keyboard_action_down &&
        shift_state == (mir_input_event_modifier_alt | mir_input_event_modifier_shift))
    {
        switch (mir_keyboard_event_scan_code(event))
        {
        case KEY_TAB:
            self->application_selector.prev(false);
            return true;

        case KEY_GRAVE:
            self->application_selector.prev(true);
            return true;

        default:;
        }
    }

    if (action == mir_keyboard_action_up)
    {
        switch (mir_keyboard_event_scan_code(event))
        {
            case KEY_LEFTALT:
                self->application_selector.complete();
                [[fallthrough]];
            default:;
        }
    }

    return false;
}

bool miral::MinimalWindowManager::handle_touch_event(MirTouchEvent const* event)
{
    if (self->handle_touch_event(event))
    {
        return true;
    }

    return false;
}

bool miral::MinimalWindowManager::handle_pointer_event(MirPointerEvent const* event)
{
    return self->handle_pointer_event(event);
}

void miral::MinimalWindowManager::handle_request_move(WindowInfo& window_info, MirInputEvent const* input_event)
{
    if (begin_pointer_move(window_info, input_event))
    {
        return;
    }
    else if (begin_touch_move(window_info, input_event))
    {
        return;
    }
}

bool miral::MinimalWindowManager::begin_pointer_move(WindowInfo const& window_info, MirInputEvent const* input_event)
{
    return self->begin_pointer_gesture(tools.info_for(window_info.window()), input_event, Gesture::pointer_moving, mir_resize_edge_none);
}

bool miral::MinimalWindowManager::begin_touch_move(WindowInfo const& window_info, MirInputEvent const* input_event)
{
    return self->begin_touch_gesture(tools.info_for(window_info.window()), input_event, Gesture::touch_moving, mir_resize_edge_none);
}

void miral::MinimalWindowManager::handle_request_resize(
    WindowInfo& window_info, MirInputEvent const* input_event, MirResizeEdge edge)
{
    if (begin_pointer_resize(window_info, input_event, edge))
    {
        return;
    }
    else if (begin_touch_resize(window_info, input_event, edge))
    {
        return;
    }
}

bool miral::MinimalWindowManager::begin_touch_resize(
    WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge)
{
    return self->begin_touch_gesture(tools.info_for(window_info.window()), input_event, Gesture::touch_resizing, edge);
}

bool miral::MinimalWindowManager::begin_pointer_resize(
    WindowInfo const& window_info, MirInputEvent const* input_event, MirResizeEdge const& edge)
{
    return self->begin_pointer_gesture(tools.info_for(window_info.window()), input_event, Gesture::pointer_resizing, edge);
}

auto miral::MinimalWindowManager::confirm_inherited_move(WindowInfo const& window_info, Displacement movement)
-> Rectangle
{
    return {window_info.window().top_left()+movement, window_info.window().size()};
}

void miral::MinimalWindowManager::advise_new_window(miral::WindowInfo const& window_info)
{
    auto should_receive_focus = self->advise_new_window(window_info);

    // If the window is prevented from stealing focus, swap the old focused
    // window (now in the back) with the new window in the front.
    //
    // If it's a legitimate window, it'll be focused and raised via
    // xdg-activation later on.
    if (!should_receive_focus)
        tools.swap_tree_order(tools.active_window(), window_info.window());
}

void miral::MinimalWindowManager::advise_focus_gained(WindowInfo const& window_info)
{
    tools.raise_tree(window_info.window());
    self->application_selector.advise_focus_gained(window_info);
}

void miral::MinimalWindowManager::advise_new_app(miral::ApplicationInfo&){}
void miral::MinimalWindowManager::advise_delete_app(miral::ApplicationInfo const&){}

void  miral::MinimalWindowManager::advise_focus_lost(const miral::WindowInfo &window_info)
{
    self->application_selector.advise_focus_lost(window_info);
}

void miral::MinimalWindowManager::advise_delete_window(miral::WindowInfo const& window_info)
{
    self->application_selector.advise_delete_window(window_info);
}

bool miral::MinimalWindowManager::Impl::prepare_for_gesture(
    WindowInfo& window_info,
    Point input_pos,
    Gesture gesture)
{
    switch (gesture)
    {
    case Gesture::pointer_moving:
    case Gesture::touch_moving:
    {
        switch (window_info.state())
        {
        case mir_window_state_restored:
            return true;

        case mir_window_state_maximized:
        case mir_window_state_vertmaximized:
        case mir_window_state_horizmaximized:
        case mir_window_state_attached:
        {
            WindowSpecification mods;
            mods.state() = mir_window_state_restored;
            tools.place_and_size_for_state(mods, window_info);
            Rectangle placement{
                mods.top_left() ? mods.top_left().value() : window_info.window().top_left(),
                mods.size() ? mods.size().value() : window_info.window().size()};
            // Keep the window's top edge in the same place
            placement.top_left.y = window_info.window().top_left().y;
            // Keep the window under the cursor/touch
            placement.top_left.x = std::min(placement.top_left.x, input_pos.x);
            placement.top_left.x = std::max(placement.top_left.x, input_pos.x - as_delta(placement.size.width));
            placement.top_left.y = std::min(placement.top_left.y, input_pos.y);
            placement.top_left.y = std::max(placement.top_left.y, input_pos.y - as_delta(placement.size.height));
            mods.top_left() = placement.top_left;
            mods.size() = placement.size;
            tools.modify_window(window_info, mods);
        }   return true;

        default: break;
        }
    }   break;

    case Gesture::pointer_resizing:
    case Gesture::touch_resizing:
        return window_info.state() == mir_window_state_restored;

    case Gesture::none:
        break;
    }

    return false;
}

bool miral::MinimalWindowManager::Impl::begin_pointer_gesture(
    WindowInfo& window_info, MirInputEvent const* input_event, Gesture gesture_, MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_pointer)
        return false;

    MirPointerEvent const* const pointer_event = mir_input_event_get_pointer_event(input_event);
    auto const position = pointer_position(pointer_event);

    if (!prepare_for_gesture(window_info, position, gesture_))
        return false;

    old_cursor = position;
    gesture = gesture_;
    gesture_window = window_info.window();
    gesture_shift_keys = mir_pointer_event_modifiers(pointer_event) & shift_states;
    resize_top_left = gesture_window.top_left();
    resize_size = gesture_window.size();
    resize_edge = edge;

    for (auto button : {mir_pointer_button_primary, mir_pointer_button_secondary, mir_pointer_button_tertiary})
    {
        if (mir_pointer_event_button_state(pointer_event, button))
        {
            pointer_gesture_button = button;
            break;
        }
    }

    return true;
}

bool miral::MinimalWindowManager::Impl::begin_touch_gesture(
    WindowInfo& window_info,
    MirInputEvent const* input_event,
    Gesture gesture_,
    MirResizeEdge edge)
{
    if (mir_input_event_get_type(input_event) != mir_input_event_type_touch)
        return false;

    MirTouchEvent const* const touch_event = mir_input_event_get_touch_event(input_event);
    auto const position = touch_center(touch_event);

    if (!prepare_for_gesture(window_info, position, gesture_))
        return false;

    old_touch = position;
    gesture = gesture_;
    gesture_window = window_info.window();
    gesture_shift_keys = mir_touch_event_modifiers(touch_event) & shift_states;
    resize_top_left = gesture_window.top_left();
    resize_size = gesture_window.size();
    resize_edge = edge;

    return true;
}

bool miral::MinimalWindowManager::Impl::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const shift_keys = mir_pointer_event_modifiers(event) & shift_states;
    auto const new_cursor = pointer_position(event);

    bool consumes_event = false;

    switch (gesture)
    {
    case Gesture::pointer_resizing:
        if (action == mir_pointer_action_motion &&
            shift_keys == gesture_shift_keys &&
            mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            apply_resize_by(new_cursor - old_cursor);
            consumes_event = true;
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    case Gesture::pointer_moving:
        if (action == mir_pointer_action_motion &&
            shift_keys == gesture_shift_keys &&
            mir_pointer_event_button_state(event, pointer_gesture_button))
        {
            if (gesture_window)
            {
                tools.drag_window(gesture_window, new_cursor - old_cursor);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    default:
        break;
    }

    if (!consumes_event && action == mir_pointer_action_button_down)
    {
        if (auto const window = tools.window_at(new_cursor))
        {
            tools.select_active_window(window);
        }

        if (auto const window = tools.active_window())
        {
            if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
            {
                if (shift_keys == pointer_drag_modifier)
                {
                    begin_pointer_gesture(
                        tools.info_for(window),
                        mir_pointer_event_input_event(event),
                        Gesture::pointer_moving, mir_resize_edge_none);
                    consumes_event = true;
                }
            }
        }
    }

    old_cursor = new_cursor;
    return consumes_event;
}

bool miral::MinimalWindowManager::Impl::handle_touch_event(MirTouchEvent const* event)
{
    bool consumes_event = false;
    auto const new_touch = touch_center(event);
    auto const count = mir_touch_event_point_count(event);
    auto const shift_keys = mir_touch_event_modifiers(event) & shift_states;

    bool is_drag = true;
    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_event_action(event, i))
        {
        case mir_touch_action_up:
        case mir_touch_action_down:
            is_drag = false;
            // Falls through
        default:
            continue;
        }
    }

    switch (gesture)
    {
    case Gesture::touch_resizing:
        if (is_drag && gesture_shift_keys == shift_keys)
        {
            if (gesture_window)
            {
                apply_resize_by(new_touch - old_touch);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    case Gesture::touch_moving:
        if (is_drag && gesture_shift_keys == shift_keys)
        {
            if (gesture_window)
            {
                tools.drag_window(gesture_window, new_touch - old_touch);
                consumes_event = true;
            }
            else
            {
                gesture = Gesture::none;
            }
        }
        else
        {
            gesture = Gesture::none;
        }
        break;

    default:
        break;
    }

    if (!consumes_event && count == 1 && mir_touch_event_action(event, 0) == mir_touch_action_down)
    {
        if (auto const window = tools.window_at(new_touch))
        {
            tools.select_active_window(window);
        }
    }

    old_touch = new_touch;
    return consumes_event;
}

void miral::MinimalWindowManager::Impl::apply_resize_by(Displacement movement)
{
    if (gesture_window)
    {
        auto const top_left = resize_top_left;
        Rectangle const old_pos{top_left, resize_size};

        auto new_width = old_pos.size.width;
        auto new_height = old_pos.size.height;

        if (resize_edge & mir_resize_edge_east)
            new_width = old_pos.size.width + movement.dx;

        if (resize_edge & mir_resize_edge_west)
            new_width = old_pos.size.width - movement.dx;

        if (resize_edge & mir_resize_edge_north)
            new_height = old_pos.size.height - movement.dy;

        if (resize_edge & mir_resize_edge_south)
            new_height = old_pos.size.height + movement.dy;

        Size new_size{new_width, new_height};

        Point new_pos = top_left;

        if (resize_edge & mir_resize_edge_west)
            new_pos.x = top_left.x + movement.dx;

        if (resize_edge & mir_resize_edge_north)
            new_pos.y = top_left.y + movement.dy;

        WindowSpecification modifications;
        modifications.top_left() = new_pos;
        modifications.size() = new_size;
        tools.modify_window(gesture_window, modifications);
        resize_top_left = new_pos;
        resize_size = new_size;
    }
    else
    {
        gesture = Gesture::none;
    }
}

bool miral::MinimalWindowManager::Impl::should_prevent_focus(miral::WindowInfo const& info)
{
    return (focus_stealing == FocusStealing::prevent) && tools.active_window() &&
           info.depth_layer() == mir_depth_layer_application &&
           tools.active_window().application() != info.window().application();
}

bool miral::MinimalWindowManager::Impl::advise_new_window(WindowInfo const& info)
{
    auto should_receive_focus = !should_prevent_focus(info);
    application_selector.advise_new_window(info, should_receive_focus);

    return should_receive_focus;
}

void miral::MinimalWindowManager::Impl::try_place_new_window_and_account_for_occlusion(WindowSpecification& parameters)
{
    namespace geom = mir::geometry;

    using RectangleSet = std::unordered_set<
                    geom::Rectangle,
                    decltype([](geom::Rectangle const& s) {
                        return std::hash<int>{}(s.size.height.as_int() * 31 + s.size.width.as_int());
                    })>;

    if (parameters.state() != mir_window_state_restored)
        return;

    RectangleSet window_rects;
    tools.for_each_application(
        [&window_rects, this](ApplicationInfo& app_info)
        {
            auto& app_windows = app_info.windows();
            auto rectangles_to_consider =
                app_windows |
                std::ranges::views::filter(
                    [this](auto& window)
                    {
                        auto const& info = tools.info_for(window);
                        // Skip invisible, windows not in the application
                        // layers, and non-floating (fullscreened or maximized)
                        // windows
                        if (!info.is_visible() || info.depth_layer() != mir_depth_layer_application ||
                            info.state() != mir_window_state_restored)
                            return false;

                        return true;
                    }) |
                std::ranges::views::transform(
                    [](Window& window)
                    {
                        return geom::Rectangle{window.top_left(), window.size()};
                    });
            window_rects.insert(rectangles_to_consider.begin(), rectangles_to_consider.end());
        });

    // Worst case scenario: you try taking a hole out of the middle of
    // a rect, results in 4 rectangles. Every other case is a special
    // case of this one where the width or height of the resulting
    // rects are < 0.
    auto const subtract_rectangles = [](geom::Rectangle rect,
                                        geom::Rectangle hole) -> std::vector<geom::Rectangle>
    {
        if (!rect.overlaps(hole))
            return {rect};

        auto const rect_a =
            geom::Rectangle{rect.top_left, geom::Size{rect.size.width, (hole.top() - rect.top()).as_int()}};

        auto const rect_b = geom::Rectangle{
            geom::Point{rect.left(), rect.top()},
            geom::Size{(hole.left() - rect.left()).as_int(), rect.size.height}};

        auto const rect_c = geom::Rectangle{
            geom::Point{hole.right(), rect.top()},
            geom::Size{(rect.right() - hole.right()).as_int(), rect.size.height}};

        auto const rect_d = geom::Rectangle{
            geom::Point{rect.left(), hole.bottom()},
            geom::Size{rect.size.width, (rect.bottom() - hole.bottom()).as_int()}};

        auto const valid_rect = [](auto const rect)
        {
            return rect.size.width.as_int() > 0 && rect.size.height.as_int() > 0;
        };
        // Funny, you can't have valid_rects be const and use `.cbegin`
        auto valid_rects = std::vector{rect_a, rect_b, rect_c, rect_d} | std::ranges::views::filter(valid_rect);

        return std::vector(valid_rects.cbegin(), valid_rects.cend());
    };

    auto const get_visible_rects =
        [subtract_rectangles](geom::Rectangle initial_test_rect, RectangleSet const& window_rects)
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
            RectangleSet iter_unoccluded_areas;
            for (auto const& test_rect : test_rects)
            {
                auto const unoccluded_areas = subtract_rectangles(test_rect, window_rect);
                iter_unoccluded_areas.insert(unoccluded_areas.begin(), unoccluded_areas.end());
            }

            test_rects = std::move(iter_unoccluded_areas);

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
                return rect.size.width.as_int() >= min_visible_width && rect.size.height.as_int() >= min_visible_height;
            });
    };

    // First, check if the given rectangle (if any) is good enough
    auto const initial_top_left = parameters.top_left().value_or({});
    auto const size = parameters.size().value_or({});
    auto const initial_rectangle = geom::Rectangle{initial_top_left, size};
    if (auto initial_position_visible_rects = get_visible_rects(initial_rectangle, window_rects);
        !initial_position_visible_rects.empty() && visible_area_large_enough(initial_position_visible_rects))
        return;

    // Try place the window around the suggested position
    auto constexpr max_iterations{16};
    auto constexpr base_offset{48};
    for (auto multiplier = 1; multiplier < max_iterations; multiplier++)
    {
        auto const offset{base_offset * multiplier};
        std::array const test_rects{
            geom::Rectangle{initial_top_left + Displacement(offset, offset), size},
            geom::Rectangle{initial_top_left + Displacement(-offset, offset), size},
            geom::Rectangle{initial_top_left + Displacement(-offset, -offset), size},
            geom::Rectangle{initial_top_left + Displacement(offset, -offset), size}};

        auto valid_test_rects = std::ranges::filter_view(
            test_rects,
            [this](auto const rectangle)
            {
                return tools.active_output().overlaps(rectangle);
            });

        for (auto const& test_rect : valid_test_rects)
        {
            auto const visible_rects = get_visible_rects(test_rect, window_rects);
            if (!visible_rects.empty() && visible_area_large_enough(visible_rects))
            {
                parameters.top_left().value() = test_rect.top_left;
                return;
            }
        }
    }

    // If we can't find any valid offset position, we use the original
    // one, possibly occluding / being occluded by another window.
}
