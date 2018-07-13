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

#include "tiling_window_manager.h"

#include <miral/application_info.h>
#include <miral/window_info.h>
#include <miral/window_manager_tools.h>
#include <miral/output.h>

#include <linux/input.h>
#include <algorithm>
#include <csignal>

namespace ms = mir::scene;
using namespace miral;

namespace
{
struct TilingWindowManagerPolicyData
{
    Rectangle tile;
    Rectangle old_tile;
};

template<class Info>
inline Rectangle& tile_for(Info& info)
{
    return std::static_pointer_cast<TilingWindowManagerPolicyData>(info.userdata())->tile;
}

template<class Info>
inline Rectangle const& tile_for(Info const& info)
{
    return std::static_pointer_cast<TilingWindowManagerPolicyData>(info.userdata())->tile;
}
}

void TilingWindowManagerPolicy::MRUTileList::push(std::shared_ptr<void> const& tile)
{
    tiles.erase(remove(begin(tiles), end(tiles), tile), end(tiles));
    tiles.push_back(tile);
}

void TilingWindowManagerPolicy::MRUTileList::erase(std::shared_ptr<void> const& tile)
{
    tiles.erase(remove(begin(tiles), end(tiles), tile), end(tiles));
}

void TilingWindowManagerPolicy::MRUTileList::enumerate(Enumerator const& enumerator) const
{
    for (auto i = tiles.rbegin(); i != tiles.rend(); ++i)
        enumerator(const_cast<std::shared_ptr<void> const&>(*i));
}

// Demonstrate implementing a simple tiling algorithm

TilingWindowManagerPolicy::TilingWindowManagerPolicy(
    WindowManagerTools const& tools,
    std::shared_ptr<SplashSession> const& spinner,
    miral::InternalClientLauncher const& launcher) :
    tools{tools},
    spinner{spinner},
    launcher{launcher}
{
}

void TilingWindowManagerPolicy::click(Point cursor)
{
    auto const window = tools.window_at(cursor);
    tools.select_active_window(window);
}

void TilingWindowManagerPolicy::resize(Point cursor)
{
    if (auto const application = application_under(cursor))
    {
        if (application == application_under(old_cursor))
        {
            if (auto const window = tools.select_active_window(tools.window_at(old_cursor)))
            {
                resize(window, cursor, old_cursor, tile_for(tools.info_for(application)));
            }
        }
    }
}

auto TilingWindowManagerPolicy::place_new_window(
    ApplicationInfo const& app_info,
    WindowSpecification const& request_parameters)
    -> WindowSpecification
{
    auto parameters = request_parameters;

    parameters.userdata() = app_info.userdata();

    if (app_info.application() != spinner->session())
    {
        Rectangle const& tile = tile_for(app_info);

        if (!parameters.parent().is_set() || !parameters.parent().value().lock())
        {
            if (app_info.windows().empty())
            {
                parameters.state() = mir_window_state_maximized;
                parameters.top_left() = tile.top_left;
                parameters.size() = tile.size;
            }
            else
            {
                parameters.state() = parameters.state().is_set() ?
                                     transform_set_state(parameters.state().value()) : mir_window_state_restored;

                auto top_level_windows = count_if(begin(app_info.windows()), end(app_info.windows()), [this]
                    (Window const& window){ return !tools.info_for(window).parent(); });

                parameters.top_left() = tile.top_left + top_level_windows*Displacement{15, 15};
            }
        }

        clip_to_tile(parameters, tile);
    }

    return parameters;
}

void TilingWindowManagerPolicy::advise_new_window(WindowInfo const& window_info)
{
    if ((window_info.type() == mir_window_type_normal || window_info.type() == mir_window_type_freestyle) &&
        !window_info.parent() &&
        (window_info.state() == mir_window_state_restored || window_info.state() == mir_window_state_maximized))
    {
        WindowSpecification specification;

        specification.state() = mir_window_state_maximized;

        tools.place_and_size_for_state(specification, window_info);
        constrain_size_and_place(specification, window_info.window(), tile_for(window_info));
        tools.modify_window(window_info.window(), specification);
    }
}

void TilingWindowManagerPolicy::handle_window_ready(WindowInfo& window_info)
{
    if (window_info.can_be_active())
        tools.select_active_window(window_info.window());

    if (spinner->session() != window_info.window().application())
    {
        tiles.push(window_info.userdata());
        dirty_tiles = true;
    }
}

namespace
{
template<typename ValueType>
void reset(mir::optional_value<ValueType>& option)
{
    if (option.is_set()) option.consume();
}
}

void TilingWindowManagerPolicy::handle_modify_window(
    miral::WindowInfo& window_info,
    miral::WindowSpecification const& modifications)
{
    auto const window = window_info.window();
    auto const tile = tile_for(window_info);
    auto mods = modifications;

    if (mods.state().is_set())
    {
        if (window_info.state() == mir_window_state_maximized &&
            (mods.parent().is_set() ? !mods.parent().value().lock() : !window_info.parent()))
        {
            mods.state() = mir_window_state_maximized;
        }
    }

    constrain_size_and_place(mods, window, tile);

    reset(mods.output_id());

    tools.modify_window(window_info, mods);
}

void TilingWindowManagerPolicy::constrain_size_and_place(
    WindowSpecification& mods, Window const& window, Rectangle const& tile) const
{
    if ((mods.state().is_set() ? mods.state().value() : tools.info_for(window).state()) == mir_window_state_maximized)
    {
        mods.top_left() = tile.top_left;
        mods.size() = tile.size;
        return;
    }

    if (mods.size().is_set())
    {
        auto width = std::min(tile.size.width, mods.size().value().width);
        auto height = std::min(tile.size.height, mods.size().value().height);

        mods.size() = Size{width, height};
    }

    if (mods.top_left().is_set())
    {
        auto x = std::max(tile.top_left.x, mods.top_left().value().x);
        auto y = std::max(tile.top_left.y, mods.top_left().value().y);

        mods.top_left() = Point{x, y};
    }

    auto top_left = mods.top_left().is_set() ? mods.top_left().value() : window.top_left();
    auto bottom_right = top_left + as_displacement(mods.size().is_set() ? mods.size().value() : window.size());
    auto overhang = bottom_right - tile.bottom_right();

    if (overhang.dx > DeltaX{0}) top_left = top_left - overhang.dx;
    if (overhang.dy > DeltaY{0}) top_left = top_left - overhang.dy;

    if (top_left != window.top_left())
        mods.top_left() = top_left;
    else
        reset(mods.top_left());
}

auto TilingWindowManagerPolicy::transform_set_state(MirWindowState value)
-> MirWindowState
{
    switch (value)
    {
    default:
        return mir_window_state_restored;

    case mir_window_state_hidden:
    case mir_window_state_minimized:
        return mir_window_state_hidden;
    }
}

void TilingWindowManagerPolicy::drag(Point cursor)
{
    if (auto const application = application_under(cursor))
    {
        if (application == application_under(old_cursor))
        {
            if (auto const window = tools.select_active_window(tools.window_at(old_cursor)))
            {

                auto const tile = tile_for(tools.info_for(application));
                WindowSpecification mods;
                mods.top_left() = window.top_left() + (cursor-old_cursor);
                constrain_size_and_place(mods, window, tile);
                tools.modify_window(window, mods);
            }
        }
    }
}

void TilingWindowManagerPolicy::handle_raise_window(WindowInfo& window_info)
{
    tools.select_active_window(window_info.window());
}

bool TilingWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

    if (action == mir_keyboard_action_down && scan_code == KEY_F11)
    {
        switch (modifiers & modifier_mask)
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

        default:
            break;
        }
    }
    else if (action == mir_keyboard_action_down && scan_code == KEY_F4)
    {
        switch (modifiers & modifier_mask)
        {
        case mir_input_event_modifier_alt|mir_input_event_modifier_shift:
            if (auto const& window = tools.active_window())
                kill(window.application(), SIGTERM);
            return true;

        case mir_input_event_modifier_alt:
            tools.ask_client_to_close(tools.active_window());;
            return true;

        default:
            break;
        }
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_TAB)
    {
        tools.focus_next_application();

        return true;
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_GRAVE)
    {
        tools.focus_next_within_application();

        return true;
    }
    else if (action == mir_keyboard_action_down &&
             modifiers == (mir_input_event_modifier_alt | mir_input_event_modifier_shift) &&
             scan_code == KEY_GRAVE)
    {
        tools.focus_prev_within_application();

        return true;
    }

    return false;
}

bool TilingWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event)
{
    auto const count = mir_touch_event_point_count(event);

    long total_x = 0;
    long total_y = 0;

    for (auto i = 0U; i != count; ++i)
    {
        total_x += mir_touch_event_axis_value(event, i, mir_touch_axis_x);
        total_y += mir_touch_event_axis_value(event, i, mir_touch_axis_y);
    }

    Point const cursor{total_x/count, total_y/count};

    bool is_drag = true;
    for (auto i = 0U; i != count; ++i)
    {
        switch (mir_touch_event_action(event, i))
        {
        case mir_touch_action_up:
            return false;

        case mir_touch_action_down:
            is_drag = false;
            continue;

        default:
            continue;
        }
    }

    bool consumes_event = false;
    if (is_drag)
    {
        switch (count)
        {
        case 4:
            resize(cursor);
            consumes_event = true;
            break;

        case 3:
            drag(cursor);
            consumes_event = true;
            break;
        }
    }
    else
    {
        if (auto const& window = tools.window_at(cursor))
            tools.select_active_window(window);
    }

    old_cursor = cursor;
    return consumes_event;
}

bool TilingWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const modifiers = mir_pointer_event_modifiers(event) & modifier_mask;
    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    bool consumes_event = false;

    if (action == mir_pointer_action_button_down)
    {
        click(cursor);
    }
    else if (action == mir_pointer_action_motion &&
             modifiers == mir_input_event_modifier_alt)
    {
        if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
        {
            drag(cursor);
            consumes_event = true;
        }
        else if (mir_pointer_event_button_state(event, mir_pointer_button_tertiary))
        {
            resize(cursor);
            consumes_event = true;
        }
    }

    old_cursor = cursor;
    return consumes_event;
}

void TilingWindowManagerPolicy::toggle(MirWindowState state)
{
    if (auto window = tools.active_window())
    {
        auto& window_info = tools.info_for(window);

        if (window_info.state() == state)
            state = mir_window_state_restored;

        WindowSpecification mods;
        mods.state() = transform_set_state(state);
        tools.modify_window(window_info, mods);
    }
}

auto TilingWindowManagerPolicy::application_under(Point position)
-> Application
{
    return tools.find_application([&, this](ApplicationInfo const& info)
        { return spinner->session() != info.application() && tile_for(info).contains(position);});
}

void TilingWindowManagerPolicy::update_tiles(Rectangles const& outputs)
{
    auto const tile_count = tiles.count();

    if (tile_count < 1 || outputs.size() < 1) return;

    auto const bounding_rect = outputs.bounding_rectangle();

    auto const total_width  = bounding_rect.size.width.as_int();
    auto const total_height = bounding_rect.size.height.as_int();

    auto index = 0;

    if (tile_count < 3)
    {
        tiles.enumerate([&](std::shared_ptr<void> const& userdata)
            {
                auto const tile_data = std::static_pointer_cast<TilingWindowManagerPolicyData>(userdata);
                tile_data->old_tile = tile_data->tile;

                auto const x = (total_width * index) / tile_count;
                ++index;
                auto const dx = (total_width * index) / tile_count - x;

                tile_data->tile = Rectangle{{x,  0}, {dx, total_height}};
            });
    }
    else
    {
        tiles.enumerate([&](std::shared_ptr<void> const& userdata)
            {
                auto const tile_data = std::static_pointer_cast<TilingWindowManagerPolicyData>(userdata);
                tile_data->old_tile = tile_data->tile;

                auto const dx = total_width/2;
                if (!index)
                {
                    tile_data->tile = Rectangle{{0,  0}, {dx, total_height}};
                }
                else
                {
                    auto const x = dx;
                    auto const y = total_height*(index-1) / (tile_count-1);
                    auto const dy = total_height / (tile_count-1);
                    tile_data->tile = Rectangle{{x,  y}, {dx, dy}};
                }

                ++index;
            });
    }

    tools.for_each_application([&](ApplicationInfo& info)
        {
            if (spinner->session() == info.application())
                return;

            auto const tile_data = std::static_pointer_cast<TilingWindowManagerPolicyData>(info.userdata());
            update_surfaces(info, tile_data->old_tile, tile_data->tile);
        });
}

void TilingWindowManagerPolicy::update_surfaces(ApplicationInfo& info, Rectangle const& old_tile, Rectangle const& new_tile)
{
    for (auto const& window : info.windows())
    {
        if (window)
        {
            auto& window_info = tools.info_for(window);

            if (!window_info.parent())
            {
                auto offset_in_tile = window.top_left() - old_tile.top_left;
                offset_in_tile.dx = std::min(offset_in_tile.dx, as_displacement(new_tile.size).dx);
                offset_in_tile.dy = std::min(offset_in_tile.dy, as_displacement(new_tile.size).dy);

                Rectangle new_placement{new_tile.top_left + offset_in_tile, window.size()};

                if (window.size().width == old_tile.size.width)
                    new_placement.size.width = new_tile.size.width;

                if (window.size().height == old_tile.size.height)
                    new_placement.size.height = new_tile.size.height;

                if (!new_placement.overlaps(new_tile))
                    new_placement.top_left = new_tile.top_left;

                new_placement = new_placement.intersection_with(new_tile);

                WindowSpecification modifications;
                modifications.top_left() = new_placement.top_left;
                modifications.size() = new_placement.size;
                tools.modify_window(window_info, modifications);
            }
        }
    }
}

void TilingWindowManagerPolicy::clip_to_tile(miral::WindowSpecification& parameters, Rectangle const& tile)
{
    auto const displacement = parameters.top_left().value() - tile.top_left;

    auto width = std::min(tile.size.width.as_int()-displacement.dx.as_int(), parameters.size().value().width.as_int());
    auto height = std::min(tile.size.height.as_int()-displacement.dy.as_int(), parameters.size().value().height.as_int());

    parameters.size() = Size{width, height};
}

void TilingWindowManagerPolicy::resize(Window window, Point cursor, Point old_cursor, Rectangle bounds)
{
    auto const top_left = window.top_left();

    auto const old_displacement = old_cursor - top_left;
    auto const new_displacement = cursor - top_left;

    auto const scale_x = float(new_displacement.dx.as_int())/std::max(1.0f, float(old_displacement.dx.as_int()));
    auto const scale_y = float(new_displacement.dy.as_int())/std::max(1.0f, float(old_displacement.dy.as_int()));

    if (scale_x <= 0.0f || scale_y <= 0.0f) return;

    auto const old_size = window.size();
    Size new_size{scale_x*old_size.width, scale_y*old_size.height};

    auto const size_limits = as_size(bounds.bottom_right() - top_left);

    if (new_size.width > size_limits.width)
        new_size.width = size_limits.width;

    if (new_size.height > size_limits.height)
        new_size.height = size_limits.height;

    window.resize(new_size);
}

void TilingWindowManagerPolicy::advise_focus_gained(WindowInfo const& info)
{
    tools.raise_tree(info.window());

    if (auto const spinner_session = spinner->session())
    {
        auto const& spinner_info = tools.info_for(spinner_session);

        if (spinner_info.windows().size() > 0)
            tools.raise_tree(spinner_info.windows()[0]);
    }
    else
    {
        tiles.push(info.userdata());
        dirty_tiles = true;
    }
}

void TilingWindowManagerPolicy::advise_new_app(miral::ApplicationInfo& application)
{
    if (spinner->session() == application.application())
        return;

    application.userdata(std::make_shared<TilingWindowManagerPolicyData>());

    // An educated guess of where the tile will be placed when the first window gets painted
    auto& tile = tile_for(application);
    tile = displays.bounding_rectangle();
    if (tiles.count() > 0)
        tile.size.width = 0.5*tile.size.width;
}

void TilingWindowManagerPolicy::advise_delete_app(miral::ApplicationInfo const& application)
{
    if (spinner->session() == application.application())
        return;

    tiles.erase(application.userdata());
    dirty_tiles = true;
}

auto TilingWindowManagerPolicy::confirm_inherited_move(miral::WindowInfo const& window_info, Displacement movement)
-> Rectangle
{
    auto const& window = window_info.window();
    WindowSpecification mods;
    mods.top_left() = window.top_left() + movement;
    constrain_size_and_place(mods, window, tile_for(window_info));
    auto pos  = mods.top_left().is_set() ? mods.top_left().value() : window.top_left();
    auto size = mods.size().is_set()     ? mods.size().value()     : window.size();
    return {pos, size};
}

void TilingWindowManagerPolicy::advise_end()
{
    if (dirty_tiles)
        update_tiles(displays);

    dirty_tiles = false;
}

void TilingWindowManagerPolicy::advise_output_create(const Output& output)
{
    displays.add(output.extents());
    dirty_tiles = true;
}

void TilingWindowManagerPolicy::advise_output_update(const Output& updated, const Output& original)
{
    if (!equivalent_display_area(updated, original))
    {
        displays.remove(original.extents());
        displays.add(updated.extents());

        dirty_tiles = true;
    }
}

void TilingWindowManagerPolicy::advise_output_delete(Output const& output)
{
    displays.remove(output.extents());
    dirty_tiles = true;
}

void TilingWindowManagerPolicy::handle_request_drag_and_drop(WindowInfo& /*window_info*/)
{
}

void TilingWindowManagerPolicy::handle_request_move(WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/)
{
}

void TilingWindowManagerPolicy::handle_request_resize(WindowInfo& /*window_info*/, MirInputEvent const* /*input_event*/, MirResizeEdge /*edge*/)
{
}

Rectangle TilingWindowManagerPolicy::confirm_placement_on_display(
    WindowInfo const& /*window_info*/,
    MirWindowState /*new_state*/,
    Rectangle const& new_placement)
{
    return new_placement; // TODO constrain this
}
