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

#include "mir/shell/canonical_window_manager.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/shell/surface_ready_observer.h"
#include "mir/shell/display_layout.h"

#include <uuid.h>
#include <linux/input.h>
#include <csignal>

#include <climits>

namespace msh = mir::shell;
namespace ms = mir::scene;
using namespace mir::geometry;

namespace
{
int const title_bar_height = 10;

// TODO this really should belong on CanonicalSurfaceInfo
// but is currently used when placing the surface before construction.
// Which implies we need some rework so that we can construct metadata
// before the surface.
bool must_not_have_parent(MirWindowType type)
{
    switch (type)
    {
    case mir_window_type_normal:
    case mir_window_type_utility:
        return true;

    default:
        return false;
    }
}

// TODO this really should belong on CanonicalSurfaceInfo
// but is currently used when placing the surface before construction.
// Which implies we need some rework so that we can construct metadata
// before the surface.
bool must_have_parent(MirWindowType type)
{
    switch (type)
    {
    case mir_window_type_gloss:;
    case mir_window_type_satellite:
    case mir_window_type_tip:
        return true;

    default:
        return false;
    }
}
}

msh::CanonicalWindowManagerPolicy::CanonicalWindowManagerPolicy(
    WindowManagerTools* const tools,
    std::shared_ptr<DisplayLayout> const& display_layout) :
    tools{tools},
    display_layout{display_layout}
{
}

void msh::CanonicalWindowManagerPolicy::click(Point cursor)
{
    if (auto const surface = tools->surface_at(cursor))
        select_active_surface(surface);

    old_cursor = cursor;
}

void msh::CanonicalWindowManagerPolicy::handle_session_info_updated(SessionInfoMap& /*session_info*/, Rectangles const& /*displays*/)
{
}

void msh::CanonicalWindowManagerPolicy::handle_displays_updated(SessionInfoMap& /*session_info*/, Rectangles const& displays)
{
    display_area = displays.bounding_rectangle();

    for (auto const weak_surface : fullscreen_surfaces)
    {
        if (auto const surface = weak_surface.lock())
        {
            auto const& info = tools->info_for(weak_surface);
            Rectangle rect{surface->top_left(), surface->size()};

            display_layout->place_in_output(info.output_id.value(), rect);
            surface->move_to(rect.top_left);
            surface->resize(rect.size);
        }
    }
}

void msh::CanonicalWindowManagerPolicy::resize(Point cursor)
{
    if (!resizing) select_active_surface(tools->surface_at(old_cursor));
    resize(active_surface(), cursor, old_cursor, display_area);
    old_cursor = cursor;
}

auto msh::CanonicalWindowManagerPolicy::handle_place_new_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& request_parameters)
-> ms::SurfaceCreationParameters
{
    auto parameters = request_parameters;

    if (!parameters.type.is_set())
        throw std::runtime_error("Surface type must be provided");

    if (!parameters.state.is_set())
        parameters.state = mir_window_state_restored;

    auto const active_display = tools->active_display();

    auto const width = parameters.size.width.as_int();
    auto const height = parameters.size.height.as_int();

    bool positioned = false;

    auto const parent = parameters.parent.lock();

    if (must_not_have_parent(parameters.type.value()) && parent)
        throw std::runtime_error("Surface type cannot have parent");

    if (must_have_parent(parameters.type.value()) && !parent)
        throw std::runtime_error("Surface type must have parent");

    if (parameters.output_id != mir::graphics::DisplayConfigurationOutputId{0})
    {
        Rectangle rect{parameters.top_left, parameters.size};
        display_layout->place_in_output(parameters.output_id, rect);
        parameters.top_left = rect.top_left;
        parameters.size = rect.size;
        parameters.state = mir_window_state_fullscreen;
        positioned = true;
    }
    else if (!parent) // No parent => client can't suggest positioning
    {
        if (auto const default_surface = session->default_surface())
        {
            static Displacement const offset{title_bar_height, title_bar_height};

            parameters.top_left = default_surface->top_left() + offset;

            geometry::Rectangle display_for_app{default_surface->top_left(), default_surface->size()};
            display_layout->size_to_output(display_for_app);

//            // TODO This is what is currently in the spec, but I think it's wrong
//            if (!display_for_app.contains(parameters.top_left + as_displacement(parameters.size)))
//            {
//                parameters.size = as_size(display_for_app.bottom_right() - parameters.top_left);
//            }
//
//            positioned = display_for_app.contains(Rectangle{parameters.top_left, parameters.size});


            positioned = display_for_app.overlaps(Rectangle{parameters.top_left, parameters.size});
        }
    }

    if (parent && parameters.aux_rect.is_set() && parameters.edge_attachment.is_set())
    {
        auto const edge_attachment = parameters.edge_attachment.value();
        auto const aux_rect = parameters.aux_rect.value();
        auto const parent_top_left = parent->top_left();
        auto const top_left = aux_rect.top_left     -Point{} + parent_top_left;
        auto const top_right= aux_rect.top_right()  -Point{} + parent_top_left;
        auto const bot_left = aux_rect.bottom_left()-Point{} + parent_top_left;

        if (edge_attachment & mir_edge_attachment_vertical)
        {
            if (active_display.contains(top_right + Displacement{width, height}))
            {
                parameters.top_left = top_right;
                positioned = true;
            }
            else if (active_display.contains(top_left + Displacement{-width, height}))
            {
                parameters.top_left = top_left + Displacement{-width, 0};
                positioned = true;
            }
        }

        if (edge_attachment & mir_edge_attachment_horizontal)
        {
            if (active_display.contains(bot_left + Displacement{width, height}))
            {
                parameters.top_left = bot_left;
                positioned = true;
            }
            else if (active_display.contains(top_left + Displacement{width, -height}))
            {
                parameters.top_left = top_left + Displacement{0, -height};
                positioned = true;
            }
        }
    }
    else if (parent)
    {
        //  o Otherwise, if the dialog is not the same as any previous dialog for the
        //    same parent window, and/or it does not have user-customized position:
        //      o It should be optically centered relative to its parent, unless this
        //        would overlap or cover the title bar of the parent.
        //      o Otherwise, it should be cascaded vertically (but not horizontally)
        //        relative to its parent, unless, this would cause at least part of
        //        it to extend into shell space.
        auto const parent_top_left = parent->top_left();
        auto const centred = parent_top_left
             + 0.5*(as_displacement(parent->size()) - as_displacement(parameters.size))
             - DeltaY{(parent->size().height.as_int()-height)/6};

        parameters.top_left = centred;
        positioned = true;
    }

    if (!positioned)
    {
        auto const centred = active_display.top_left
            + 0.5*(as_displacement(active_display.size) - as_displacement(parameters.size))
            - DeltaY{(active_display.size.height.as_int()-height)/6};

        switch (parameters.state.value())
        {
        case mir_window_state_fullscreen:
        case mir_window_state_maximized:
            parameters.top_left = active_display.top_left;
            parameters.size = active_display.size;
            break;

        case mir_window_state_vertmaximized:
            parameters.top_left = centred;
            parameters.top_left.y = active_display.top_left.y;
            parameters.size.height = active_display.size.height;
            break;

        case mir_window_state_horizmaximized:
            parameters.top_left = centred;
            parameters.top_left.x = active_display.top_left.x;
            parameters.size.width = active_display.size.width;
            break;

        default:
            parameters.top_left = centred;
        }

        if (parameters.top_left.y < display_area.top_left.y)
            parameters.top_left.y = display_area.top_left.y;
    }

    return parameters;
}

void msh::CanonicalWindowManagerPolicy::handle_new_surface(std::shared_ptr<ms::Session> const& session, std::shared_ptr<ms::Surface> const& surface)
{
    auto& surface_info = tools->info_for(surface);
    if (auto const parent = surface_info.parent.lock())
    {
        tools->info_for(parent).children.push_back(surface);
    }

    tools->info_for(session).surfaces.push_back(surface);

    if (surface_info.can_be_active())
    {
        surface->add_observer(std::make_shared<shell::SurfaceReadyObserver>(
            [this](std::shared_ptr<scene::Session> const& /*session*/,
                   std::shared_ptr<scene::Surface> const& surface)
                {
                    select_active_surface(surface);
                },
            session,
            surface));
    }

    if (surface_info.state == mir_window_state_fullscreen)
        fullscreen_surfaces.insert(surface);
}

void msh::CanonicalWindowManagerPolicy::handle_modify_surface(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    SurfaceSpecification const& modifications)
{
    auto& surface_info_old = tools->info_for(surface);

    auto surface_info = surface_info_old;

    if (modifications.parent.is_set())
        surface_info.parent = modifications.parent.value();

    if (modifications.type.is_set() &&
        surface_info.type != modifications.type.value())
    {
        auto const new_type = modifications.type.value();

        if (!surface_info.can_morph_to(new_type))
        {
            throw std::runtime_error("Unsupported surface type change");
        }

        surface_info.type = new_type;

        if (surface_info.must_not_have_parent())
        {
            if (modifications.parent.is_set())
                throw std::runtime_error("Target surface type does not support parent");

            surface_info.parent.reset();
        }
        else if (surface_info.must_have_parent())
        {
            if (!surface_info.parent.lock())
                throw std::runtime_error("Target surface type requires parent");
        }

        surface->configure(mir_window_attrib_type, new_type);
    }

    #define COPY_IF_SET(field)\
        if (modifications.field.is_set())\
            surface_info.field = modifications.field.value()

    COPY_IF_SET(min_width);
    COPY_IF_SET(min_height);
    COPY_IF_SET(max_width);
    COPY_IF_SET(max_height);
    COPY_IF_SET(min_width);
    COPY_IF_SET(width_inc);
    COPY_IF_SET(height_inc);
    COPY_IF_SET(min_aspect);
    COPY_IF_SET(max_aspect);
    COPY_IF_SET(output_id);
    COPY_IF_SET(confine_pointer);

    #undef COPY_IF_SET

    std::swap(surface_info, surface_info_old);

    if (modifications.name.is_set())
        surface->rename(modifications.name.value());

    if (modifications.streams.is_set())
    {
        auto v = modifications.streams.value();
        std::vector<shell::StreamSpecification> l (v.begin(), v.end());
        session->configure_streams(*surface, l);
    }

    if (modifications.width.is_set() || modifications.height.is_set())
    {
        auto new_size = surface->size();

        if (modifications.width.is_set())
            new_size.width = modifications.width.value();

        if (modifications.height.is_set())
            new_size.height = modifications.height.value();

        auto top_left = surface->top_left();

        surface_info.constrain_resize(
            surface,
            top_left,
            new_size,
            false,
            false,
            display_area);

        surface->resize(new_size);
    }

    if (modifications.input_shape.is_set())
    {
        auto rectangles = modifications.input_shape.value();
        auto displacement = surface->top_left() - Point{0, 0}; 
        for(auto& rect : rectangles)
        {
            rect.top_left = rect.top_left + displacement;
            rect = rect.intersection_with({surface->top_left(), surface->size()});
            rect.top_left = rect.top_left - displacement;
        }
        surface->set_input_region(rectangles);
    }

    if (modifications.width.is_set() || modifications.height.is_set())
    {
        move_tree(surface, {});
    }

    if (modifications.state.is_set())
    {
        auto const state = handle_set_state(surface, modifications.state.value());
        surface->configure(mir_window_attrib_state, state);
    }

    if (modifications.confine_pointer.is_set())
    {
        surface->set_confine_pointer_state(modifications.confine_pointer.value());
    }
}

void msh::CanonicalWindowManagerPolicy::handle_delete_surface(std::shared_ptr<ms::Session> const& session, std::weak_ptr<ms::Surface> const& surface)
{
    fullscreen_surfaces.erase(surface);
    bool const is_active_surface{surface.lock() == active_surface()};

    auto& info = tools->info_for(surface);

    if (auto const parent = info.parent.lock())
    {
        auto& siblings = tools->info_for(parent).children;

        for (auto i = begin(siblings); i != end(siblings); ++i)
        {
            if (surface.lock() == i->lock())
            {
                siblings.erase(i);
                break;
            }
        }
    }

    session->destroy_surface(surface);

    auto& surfaces = tools->info_for(session).surfaces;

    for (auto i = begin(surfaces); i != end(surfaces); ++i)
    {
        if (surface.lock() == i->lock())
        {
            surfaces.erase(i);
            break;
        }
    }

    if (is_active_surface)
    {
        active_surface_.reset();

        if (surfaces.empty())
        {
            tools->focus_next_session();
            select_active_surface(tools->focused_surface());
        }
        else
        {
            select_active_surface(surfaces[0].lock());
        }
    }
}

int msh::CanonicalWindowManagerPolicy::handle_set_state(std::shared_ptr<ms::Surface> const& surface, MirWindowState value)
{
    auto& info = tools->info_for(surface);

    switch (value)
    {
    case mir_window_state_restored:
    case mir_window_state_maximized:
    case mir_window_state_vertmaximized:
    case mir_window_state_horizmaximized:
    case mir_window_state_fullscreen:
    case mir_window_state_hidden:
    case mir_window_state_minimized:
        break;

    default:
        return info.state;
    }

    if (info.state == mir_window_state_restored)
    {
        info.restore_rect = {surface->top_left(), surface->size()};
    }

    if (info.state != mir_window_state_fullscreen)
    {
        info.output_id = decltype(info.output_id){};
        fullscreen_surfaces.erase(surface);
    }
    else
    {
        fullscreen_surfaces.insert(surface);
    }

    if (info.state == value)
    {
        return info.state;
    }

    auto const old_pos = surface->top_left();
    Displacement movement;

    switch (value)
    {
    case mir_window_state_restored:
        movement = info.restore_rect.top_left - old_pos;
        surface->resize(info.restore_rect.size);
        break;

    case mir_window_state_maximized:
        movement = display_area.top_left - old_pos;
        surface->resize(display_area.size);
        break;

    case mir_window_state_horizmaximized:
        movement = Point{display_area.top_left.x, info.restore_rect.top_left.y} - old_pos;
        surface->resize({display_area.size.width, info.restore_rect.size.height});
        break;

    case mir_window_state_vertmaximized:
        movement = Point{info.restore_rect.top_left.x, display_area.top_left.y} - old_pos;
        surface->resize({info.restore_rect.size.width, display_area.size.height});
        break;

    case mir_window_state_fullscreen:
    {
        Rectangle rect{old_pos, surface->size()};

        if (info.output_id.is_set())
        {
            display_layout->place_in_output(info.output_id.value(), rect);
        }
        else
        {
            display_layout->size_to_output(rect);
        }

        movement = rect.top_left - old_pos;
        surface->resize(rect.size);
        break;
    }

    case mir_window_state_hidden:
    case mir_window_state_minimized:
        surface->hide();
        return info.state = value;

    default:
        break;
    }

    // TODO It is rather simplistic to move a tree WRT the top_left of the root
    // TODO when resizing. But for more sophistication we would need to encode
    // TODO some sensible layout rules.
    move_tree(surface, movement);

    info.state = value;

    if (info.is_visible())
        surface->show();

    return info.state;
}

void msh::CanonicalWindowManagerPolicy::drag(Point cursor)
{
    select_active_surface(tools->surface_at(old_cursor));
    drag(active_surface(), cursor, old_cursor, display_area);
    old_cursor = cursor;
}

void msh::CanonicalWindowManagerPolicy::handle_raise_surface(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface)
{
    select_active_surface(surface);
}

void msh::CanonicalWindowManagerPolicy::handle_request_drag_and_drop(
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface)
{
    uuid_t uuid;
    uuid_generate(uuid);
    std::vector<uint8_t> const handle{std::begin(uuid), std::end(uuid)};

    surface->start_drag_and_drop(handle);
    tools->set_drag_and_drop_handle(handle);
}

bool msh::CanonicalWindowManagerPolicy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

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

        default:
            break;
        }
    }
    else if (action == mir_keyboard_action_down && scan_code == KEY_F4)
    {
        if (auto const session = tools->focused_session())
        {
            switch (modifiers)
            {
            case mir_input_event_modifier_alt:
                kill(session->process_id(), SIGTERM);
                return true;

            case mir_input_event_modifier_ctrl:
                if (auto const surf = session->default_surface())
                {
                    surf->request_client_surface_close();
                    return true;
                }

            default:
                break;
            }
        }
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_TAB)
    {
        tools->focus_next_session();
        if (auto const surface = tools->focused_surface())
            select_active_surface(surface);

        return true;
    }
    else if (action == mir_keyboard_action_down &&
            modifiers == mir_input_event_modifier_alt &&
            scan_code == KEY_GRAVE)
    {
        if (auto const prev = tools->focused_surface())
        {
            if (auto const app = tools->focused_session())
                select_active_surface(app->surface_after(prev));
        }

        return true;
    }

    return false;
}

bool msh::CanonicalWindowManagerPolicy::handle_touch_event(MirTouchEvent const* event)
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
            // fallthrough
        case mir_touch_action_change:
            continue;

        case mir_touch_actions:
            abort();
            return false;
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

    old_cursor = cursor;
    return consumes_event;
}

bool msh::CanonicalWindowManagerPolicy::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const modifiers = mir_pointer_event_modifiers(event) & modifier_mask;
    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    bool consumes_event = false;
    bool resize_event = false;

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

        if (mir_pointer_event_button_state(event, mir_pointer_button_tertiary))
        {
            resize(cursor);
            resize_event = active_surface_.lock().get();
            consumes_event = true;
        }
    }

    resizing = resize_event;
    old_cursor = cursor;
    return consumes_event;
}

void msh::CanonicalWindowManagerPolicy::toggle(MirWindowState state)
{
    if (auto const surface = active_surface())
    {
        auto& info = tools->info_for(surface);

        if (info.state == state)
            state = mir_window_state_restored;

        auto const value = handle_set_state(surface, MirWindowState(state));
        surface->configure(mir_window_attrib_state, value);
    }
}

void msh::CanonicalWindowManagerPolicy::select_active_surface(std::shared_ptr<ms::Surface> const& surface)
{
    std::lock_guard<std::recursive_mutex> lock{active_surface_mutex};

    if (!surface)
    {
        if (active_surface_.lock())
            tools->set_focus_to({}, {});

        active_surface_.reset();
        return;
    }

    auto const& info_for = tools->info_for(surface);

    if (info_for.can_be_active())
    {
        tools->set_focus_to(info_for.session.lock(), surface);
        tools->raise_tree(surface);
        active_surface_ = surface;
    }
    else
    {
        // Cannot have input focus - try the parent
        if (auto const parent = info_for.parent.lock())
            select_active_surface(parent);
    }
}

auto msh::CanonicalWindowManagerPolicy::active_surface() const
-> std::shared_ptr<ms::Surface>
{
    if (auto const surface = active_surface_.lock())
        return surface;

    if (auto const session = tools->focused_session())
    {
        if (auto const surface = session->default_surface())
            return surface;
    }

    return std::shared_ptr<ms::Surface>{};
}

bool msh::CanonicalWindowManagerPolicy::resize(std::shared_ptr<ms::Surface> const& surface, Point cursor, Point old_cursor, Rectangle bounds)
{
    if (!surface)
        return false;

    auto const& surface_info = tools->info_for(surface);

    auto const top_left = surface->top_left();
    Rectangle const old_pos{top_left, surface->size()};

    if (!resizing)
    {
        auto anchor = old_pos.bottom_right();

        for (auto const& corner : {
            old_pos.top_right(),
            old_pos.bottom_left(),
            top_left})
        {
            if ((old_cursor - anchor).length_squared() <
                (old_cursor - corner).length_squared())
            {
                anchor = corner;
            }
        }

        left_resize = anchor.x != top_left.x;
        top_resize  = anchor.y != top_left.y;
    }

    int const x_sign = left_resize? -1 : 1;
    int const y_sign = top_resize?  -1 : 1;

    auto delta = cursor-old_cursor;

    auto new_width = old_pos.size.width + x_sign * delta.dx;
    auto new_height = old_pos.size.height + y_sign * delta.dy;

    auto const min_width  = std::max(surface_info.min_width, Width{5});
    auto const min_height = std::max(surface_info.min_height, Height{5});

    if (new_width < min_width)
    {
        new_width = min_width;
        if (delta.dx > DeltaX{0})
            delta.dx = DeltaX{0};
    }

    if (new_height < min_height)
    {
        new_height = min_height;
        if (delta.dy > DeltaY{0})
            delta.dy = DeltaY{0};
    }

    Size new_size{new_width, new_height};
    Point new_pos = top_left + left_resize*delta.dx + top_resize*delta.dy;


    surface_info.constrain_resize(surface, new_pos, new_size, left_resize, top_resize, bounds);

    apply_resize(surface, new_pos, new_size);

    return true;
}

void msh::CanonicalWindowManagerPolicy::apply_resize(
    std::shared_ptr<ms::Surface> const& surface,
    Point const& new_pos,
    Size const& new_size) const
{
    surface->resize(new_size);

    // TODO It is rather simplistic to move a tree WRT the top_left of the root
    // TODO when resizing. But for more sophistication we would need to encode
    // TODO some sensible layout rules.
    move_tree(surface, new_pos-surface->top_left());
}

bool msh::CanonicalWindowManagerPolicy::drag(std::shared_ptr<ms::Surface> surface, Point to, Point from, Rectangle /*bounds*/)
{
    if (!surface)
        return false;

    if (!surface->input_area_contains(from))
        return false;

    auto movement = to - from;

    // placeholder - constrain onscreen

    switch (tools->info_for(surface).state)
    {
    case mir_window_state_restored:
        break;

    // "A vertically maximised surface is anchored to the top and bottom of
    // the available workspace and can have any width."
    case mir_window_state_vertmaximized:
        movement.dy = DeltaY(0);
        break;

    // "A horizontally maximised surface is anchored to the left and right of
    // the available workspace and can have any height"
    case mir_window_state_horizmaximized:
        movement.dx = DeltaX(0);
        break;

    // "A maximised surface is anchored to the top, bottom, left and right of the
    // available workspace. For example, if the launcher is always-visible then
    // the left-edge of the surface is anchored to the right-edge of the launcher."
    case mir_window_state_maximized:
    case mir_window_state_fullscreen:
    default:
        return true;
    }

    move_tree(surface, movement);

    return true;
}

void msh::CanonicalWindowManagerPolicy::move_tree(std::shared_ptr<ms::Surface> const& root, Displacement movement) const
{
    root->move_to(root->top_left() + movement);

    for (auto const& child: tools->info_for(root).children)
    {
        move_tree(child.lock(), movement);
    }
}
