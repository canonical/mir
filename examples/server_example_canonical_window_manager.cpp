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

#include "server_example_canonical_window_manager.h"

#include "mir/scene/surface.h"
#include "mir/shell/surface_ready_observer.h"
#include "mir/shell/display_layout.h"
#include "mir/shell/surface_specification.h"
#include "mir/geometry/displacement.h"

#include "mir/graphics/buffer.h"

#include <linux/input.h>
#include <csignal>
#include <algorithm>

namespace me = mir::examples;
namespace ms = mir::scene;
using namespace mir::geometry;

///\example server_example_canonical_window_manager.cpp
// Based on "Mir and Unity: Surfaces, input, and displays (v0.3)"

namespace
{
int const title_bar_height = 10;
Size titlebar_size_for_window(Size window_size)
{
    return {window_size.width, Height{title_bar_height}};
}

Point titlebar_position_for_window(Point window_position)
{
    return {
        window_position.x,
        window_position.y - DeltaY(title_bar_height)
    };
}

bool needs_titlebar(MirSurfaceType type)
{
    switch (type)
    {
    case mir_surface_type_freestyle:
    case mir_surface_type_menu:
    case mir_surface_type_inputmethod:
    case mir_surface_type_gloss:
    case mir_surface_type_tip:
        // No decorations for these surface types
        return false;
    default:
        return true;
    }
}
}

me::CanonicalSurfaceInfoCopy::CanonicalSurfaceInfoCopy(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    scene::SurfaceCreationParameters const& params) :
    type{surface->type()},
    state{surface->state()},
    restore_rect{surface->top_left(), surface->size()},
    session{session},
    parent{params.parent},
    min_width{params.min_width.is_set() ? params.min_width.value()  : Width{}},
    min_height{params.min_height.is_set() ? params.min_height.value() : Height{}},
    max_width{params.max_width.is_set() ? params.max_width.value() : Width{std::numeric_limits<int>::max()}},
    max_height{params.max_height.is_set() ? params.max_height.value() : Height{std::numeric_limits<int>::max()}},
    width_inc{params.width_inc},
    height_inc{params.height_inc},
    min_aspect{params.min_aspect},
    max_aspect{params.max_aspect}
{
}

bool me::CanonicalSurfaceInfoCopy::can_be_active() const
{
    switch (type)
    {
    case mir_surface_type_normal:       /**< AKA "regular"                       */
    case mir_surface_type_utility:      /**< AKA "floating"                      */
    case mir_surface_type_dialog:
    case mir_surface_type_satellite:    /**< AKA "toolbox"/"toolbar"             */
    case mir_surface_type_freestyle:
    case mir_surface_type_menu:
    case mir_surface_type_inputmethod:  /**< AKA "OSK" or handwriting etc.       */
        return true;

    case mir_surface_type_gloss:
    case mir_surface_type_tip:          /**< AKA "tooltip"                       */
    default:
        // Cannot have input focus
        return false;
    }
}

bool me::CanonicalSurfaceInfoCopy::must_have_parent() const
{
    switch (type)
    {
    case mir_surface_type_overlay:;
    case mir_surface_type_inputmethod:
    case mir_surface_type_satellite:
    case mir_surface_type_tip:
        return true;

    default:
        return false;
    }
}

bool me::CanonicalSurfaceInfoCopy::can_morph_to(MirSurfaceType new_type) const
{
    switch (new_type)
    {
    case mir_surface_type_normal:
    case mir_surface_type_utility:
    case mir_surface_type_satellite:
        switch (type)
        {
        case mir_surface_type_normal:
        case mir_surface_type_utility:
        case mir_surface_type_dialog:
        case mir_surface_type_satellite:
            return true;

        default:
            break;
        }
        break;

    case mir_surface_type_dialog:
        switch (type)
        {
        case mir_surface_type_normal:
        case mir_surface_type_utility:
        case mir_surface_type_dialog:
        case mir_surface_type_popover:
        case mir_surface_type_satellite:
            return true;

        default:
            break;
        }
        break;

    default:
        break;
    }

    return false;
}

bool me::CanonicalSurfaceInfoCopy::must_not_have_parent() const
{
    switch (type)
    {
    case mir_surface_type_normal:
    case mir_surface_type_utility:
        return true;

    default:
        return false;
    }
}


me::CanonicalWindowManagerPolicyCopy::CanonicalWindowManagerPolicyCopy(
    Tools* const tools,
    std::shared_ptr<shell::DisplayLayout> const& display_layout) :
    tools{tools},
    display_layout{display_layout}
{
}

void me::CanonicalWindowManagerPolicyCopy::click(Point cursor)
{
    if (auto const surface = tools->surface_at(cursor))
        select_active_surface(surface);

    old_cursor = cursor;
}

void me::CanonicalWindowManagerPolicyCopy::handle_session_info_updated(CanonicalSessionInfoMap& /*session_info*/, Rectangles const& /*displays*/)
{
}

void me::CanonicalWindowManagerPolicyCopy::handle_displays_updated(CanonicalSessionInfoMap& /*session_info*/, Rectangles const& displays)
{
    display_area = displays.bounding_rectangle();
}

void me::CanonicalWindowManagerPolicyCopy::resize(Point cursor)
{
    select_active_surface(tools->surface_at(old_cursor));
    resize(active_surface(), cursor, old_cursor, display_area);
    old_cursor = cursor;
}

auto me::CanonicalWindowManagerPolicyCopy::handle_place_new_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& request_parameters)
-> ms::SurfaceCreationParameters
{
    auto parameters = request_parameters;
    auto surf_type = parameters.type.is_set() ? parameters.type.value() : mir_surface_type_normal;
    bool const needs_titlebar = ::needs_titlebar(surf_type);

    if (needs_titlebar)
        parameters.size.height = parameters.size.height + DeltaY{title_bar_height};

    if (!parameters.state.is_set())
        parameters.state = mir_surface_state_restored;

    auto const active_display = tools->active_display();

    auto const width = parameters.size.width.as_int();
    auto const height = parameters.size.height.as_int();

    bool positioned = false;

    auto const parent = parameters.parent.lock();

    if (parameters.output_id != mir::graphics::DisplayConfigurationOutputId{0})
    {
        Rectangle rect{parameters.top_left, parameters.size};
        display_layout->place_in_output(parameters.output_id, rect);
        parameters.top_left = rect.top_left;
        parameters.size = rect.size;
        parameters.state = mir_surface_state_fullscreen;
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
        case mir_surface_state_fullscreen:
        case mir_surface_state_maximized:
            parameters.top_left = active_display.top_left;
            parameters.size = active_display.size;
            break;

        case mir_surface_state_vertmaximized:
            parameters.top_left = centred;
            parameters.top_left.y = active_display.top_left.y;
            parameters.size.height = active_display.size.height;
            break;

        case mir_surface_state_horizmaximized:
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

    if (parameters.state != mir_surface_state_fullscreen && needs_titlebar)
    {
        parameters.top_left.y = parameters.top_left.y + DeltaY{title_bar_height};
        parameters.size.height = parameters.size.height - DeltaY{title_bar_height};
    }

    return parameters;
}

//TODO: provide an easier way for the server to write to a surface!
//TODO: this is painful to use mg::Buffer::write()
struct mir::examples::CanonicalSurfaceInfoCopy::PaintingImpl
{
    PaintingImpl(std::shared_ptr<frontend::BufferStream> const& buffer_stream) :
        buffer_stream{buffer_stream}, buffer{nullptr}
    {
        swap_buffers();
    }

    void swap_buffers()
    {
        auto const callback = [this](mir::graphics::Buffer* new_buffer)
            {
                buffer.store(new_buffer);
            };

        buffer_stream->swap_buffers(buffer, callback);
    }

    std::shared_ptr<frontend::BufferStream> const buffer_stream;
    std::atomic<graphics::Buffer*> buffer;
};

void mir::examples::CanonicalSurfaceInfoCopy::init_titlebar(std::shared_ptr<scene::Surface> const& surface)
{
    painting_impl = std::make_shared<PaintingImpl>(surface->primary_buffer_stream());
}

void mir::examples::CanonicalSurfaceInfoCopy::paint_titlebar(int intensity)
{
    if (auto const buf = painting_impl->buffer.load())
    {
        auto const format = painting_impl->buffer_stream->pixel_format();
        auto const sz = buf->size().height.as_int() *
            buf->size().width.as_int() * MIR_BYTES_PER_PIXEL(format);

        std::vector<unsigned char> pixels(sz, intensity);
        buf->write(pixels.data(), sz);

        painting_impl->swap_buffers();
    }
}

void me::CanonicalWindowManagerPolicyCopy::generate_decorations_for(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    CanonicalSurfaceInfoMap& surface_map)
{
    if (!needs_titlebar(surface->type()))
        return;

    auto format = mir_pixel_format_xrgb_8888;
    ms::SurfaceCreationParameters params;
    params.of_size(titlebar_size_for_window(surface->size()))
        .of_name("decoration")
        .of_pixel_format(format)
        .of_buffer_usage(mir::graphics::BufferUsage::software)
        .of_position(titlebar_position_for_window(surface->top_left()))
        .of_type(mir_surface_type_gloss);
    auto id = session->create_surface(params);
    auto titlebar = session->surface(id);
    titlebar->set_alpha(0.9);

    auto& surface_info = tools->info_for(surface);
    surface_info.titlebar = titlebar;
    surface_info.titlebar_id = id;
    surface_info.children.push_back(titlebar);

    CanonicalSurfaceInfoCopy& titlebar_info =
            surface_map.emplace(titlebar, CanonicalSurfaceInfoCopy{session, titlebar, {}}).first->second;
    titlebar_info.is_titlebar = true;
    titlebar_info.parent = surface;
    titlebar_info.init_titlebar(titlebar);
}

void me::CanonicalWindowManagerPolicyCopy::handle_new_surface(std::shared_ptr<ms::Session> const& session, std::shared_ptr<ms::Surface> const& surface)
{
    auto& surface_info = tools->info_for(surface);
    if (auto const parent = surface_info.parent.lock())
    {
        tools->info_for(parent).children.push_back(surface);
    }

    tools->info_for(session).surfaces++;

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
}

void me::CanonicalWindowManagerPolicyCopy::handle_modify_surface(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    shell::SurfaceSpecification const& modifications)
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

        surface->configure(mir_surface_attrib_type, new_type);
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

    if (modifications.input_shape.is_set())
    {
        surface->set_input_region(modifications.input_shape.value());
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

        apply_resize(surface, surface_info.titlebar, top_left, new_size);
    }
}

void me::CanonicalWindowManagerPolicyCopy::handle_delete_surface(std::shared_ptr<ms::Session> const& session, std::weak_ptr<ms::Surface> const& surface)
{
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

    if (info.titlebar)
    {
        tools->forget(info.titlebar);
        session->destroy_surface(info.titlebar_id);
    }

    if (!--tools->info_for(session).surfaces && session == tools->focused_session())
    {
        active_surface_.reset();
        tools->focus_next_session();
        select_active_surface(tools->focused_surface());
    }
}

int me::CanonicalWindowManagerPolicyCopy::handle_set_state(std::shared_ptr<ms::Surface> const& surface, MirSurfaceState value)
{
    auto& info = tools->info_for(surface);

    switch (value)
    {
    case mir_surface_state_restored:
    case mir_surface_state_maximized:
    case mir_surface_state_vertmaximized:
    case mir_surface_state_horizmaximized:
    case mir_surface_state_fullscreen:
    case mir_surface_state_hidden:
    case mir_surface_state_minimized:
        break;

    default:
        return info.state;
    }

    if (info.state == mir_surface_state_restored)
    {
        info.restore_rect = {surface->top_left(), surface->size()};
    }

    if (info.state == value)
    {
        return info.state;
    }

    auto const old_pos = surface->top_left();
    Displacement movement;

    switch (value)
    {
    case mir_surface_state_restored:
        movement = info.restore_rect.top_left - old_pos;
        surface->resize(info.restore_rect.size);
        if (info.titlebar)
        {
            info.titlebar->resize(titlebar_size_for_window(info.restore_rect.size));
            info.titlebar->show();
        }
        break;

    case mir_surface_state_maximized:
        movement = display_area.top_left - old_pos;
        surface->resize(display_area.size);
        if (info.titlebar)
            info.titlebar->hide();
        break;

    case mir_surface_state_horizmaximized:
        movement = Point{display_area.top_left.x, info.restore_rect.top_left.y} - old_pos;
        surface->resize({display_area.size.width, info.restore_rect.size.height});
        if (info.titlebar)
        {
            info.titlebar->resize(titlebar_size_for_window({display_area.size.width, info.restore_rect.size.height}));
            info.titlebar->show();
        }
        break;

    case mir_surface_state_vertmaximized:
        movement = Point{info.restore_rect.top_left.x, display_area.top_left.y} - old_pos;
        surface->resize({info.restore_rect.size.width, display_area.size.height});
        if (info.titlebar)
            info.titlebar->hide();
        break;

    case mir_surface_state_fullscreen:
    {
        Rectangle rect{old_pos, surface->size()};
        display_layout->size_to_output(rect);
        movement = rect.top_left - old_pos;
        surface->resize(rect.size);
        break;
    }

    case mir_surface_state_hidden:
    case mir_surface_state_minimized:
        if (info.titlebar)
            info.titlebar->hide();
        surface->hide();
        return info.state = value;

    default:
        break;
    }

    // TODO It is rather simplistic to move a tree WRT the top_left of the root
    // TODO when resizing. But for more sophistication we would need to encode
    // TODO some sensible layout rules.
    move_tree(surface, movement);

    return info.state = value;
}

void me::CanonicalWindowManagerPolicyCopy::drag(Point cursor)
{
    select_active_surface(tools->surface_at(old_cursor));
    drag(active_surface(), cursor, old_cursor, display_area);
    old_cursor = cursor;
}

bool me::CanonicalWindowManagerPolicyCopy::handle_keyboard_event(MirKeyboardEvent const* event)
{
    auto const action = mir_keyboard_event_action(event);
    auto const scan_code = mir_keyboard_event_scan_code(event);
    auto const modifiers = mir_keyboard_event_modifiers(event) & modifier_mask;

    if (action == mir_keyboard_action_down && scan_code == KEY_F11)
    {
        switch (modifiers)
        {
        case mir_input_event_modifier_alt:
            toggle(mir_surface_state_maximized);
            return true;

        case mir_input_event_modifier_shift:
            toggle(mir_surface_state_vertmaximized);
            return true;

        case mir_input_event_modifier_ctrl:
            toggle(mir_surface_state_horizmaximized);
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

bool me::CanonicalWindowManagerPolicyCopy::handle_touch_event(MirTouchEvent const* event)
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

        case mir_touch_action_change:
            continue;
        }
    }

    if (is_drag && count == 3)
    {
        drag(cursor);
        return true;
    }
    else
    {
        click(cursor);
        return false;
    }
}

bool me::CanonicalWindowManagerPolicyCopy::handle_pointer_event(MirPointerEvent const* event)
{
    auto const action = mir_pointer_event_action(event);
    auto const modifiers = mir_pointer_event_modifiers(event) & modifier_mask;
    Point const cursor{
        mir_pointer_event_axis_value(event, mir_pointer_axis_x),
        mir_pointer_event_axis_value(event, mir_pointer_axis_y)};

    if (action == mir_pointer_action_button_down)
    {
        click(cursor);
        return false;
    }
    else if (action == mir_pointer_action_motion &&
             modifiers == mir_input_event_modifier_alt)
    {
        if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
        {
            drag(cursor);
            return true;
        }

        if (mir_pointer_event_button_state(event, mir_pointer_button_tertiary))
        {
            resize(cursor);
            return true;
        }
    }
    else if (action == mir_pointer_action_motion && !modifiers)
    {
        if (mir_pointer_event_button_state(event, mir_pointer_button_primary))
        {
            if (auto const possible_titlebar = tools->surface_at(old_cursor))
            {
                if (tools->info_for(possible_titlebar).is_titlebar)
                {
                    drag(cursor);
                    return true;
                }
            }
        }
    }

    return false;
}

void me::CanonicalWindowManagerPolicyCopy::toggle(MirSurfaceState state)
{
    if (auto const surface = active_surface())
    {
        auto& info = tools->info_for(surface);

        if (info.state == state)
            state = mir_surface_state_restored;

        auto const value = handle_set_state(surface, MirSurfaceState(state));
        surface->configure(mir_surface_attrib_state, value);
    }
}

void me::CanonicalWindowManagerPolicyCopy::select_active_surface(std::shared_ptr<ms::Surface> const& surface)
{
    if (surface == active_surface_.lock())
        return;

    if (!surface)
    {
        if (auto const active_surface = active_surface_.lock())
        {
            if (auto const titlebar = tools->info_for(active_surface).titlebar)
            {
                tools->info_for(titlebar).paint_titlebar(0x3F);
            }
        }

        if (active_surface_.lock())
            tools->set_focus_to({}, {});

        active_surface_.reset();
        return;
    }

    auto const& info_for = tools->info_for(surface);

    if (info_for.can_be_active())
    {
        if (auto const active_surface = active_surface_.lock())
        {
            if (auto const titlebar = tools->info_for(active_surface).titlebar)
            {
                tools->info_for(titlebar).paint_titlebar(0x3F);
            }
        }
        if (auto const titlebar = tools->info_for(surface).titlebar)
        {
            tools->info_for(titlebar).paint_titlebar(0xFF);
        }
        tools->set_focus_to(info_for.session.lock(), surface);
        raise_tree(surface);
        active_surface_ = surface;
    }
    else
    {
        // Cannot have input focus - try the parent
        if (auto const parent = info_for.parent.lock())
            select_active_surface(parent);
    }
}

auto me::CanonicalWindowManagerPolicyCopy::active_surface() const
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

bool me::CanonicalWindowManagerPolicyCopy::resize(std::shared_ptr<ms::Surface> const& surface, Point cursor, Point old_cursor, Rectangle bounds)
{
    if (!surface || !surface->input_area_contains(old_cursor))
        return false;

    auto const top_left = surface->top_left();
    Rectangle const old_pos{top_left, surface->size()};

    auto anchor = top_left;

    for (auto const& corner : {
        old_pos.top_right(),
        old_pos.bottom_left(),
        old_pos.bottom_right()})
    {
        if ((old_cursor - anchor).length_squared() <
            (old_cursor - corner).length_squared())
        {
            anchor = corner;
        }
    }

    bool const left_resize = anchor.x != top_left.x;
    bool const top_resize  = anchor.y != top_left.y;
    int const x_sign = left_resize? -1 : 1;
    int const y_sign = top_resize?  -1 : 1;

    auto const delta = cursor-old_cursor;

    Size new_size{old_pos.size.width + x_sign*delta.dx, old_pos.size.height + y_sign*delta.dy};

    Point new_pos = top_left + left_resize*delta.dx + top_resize*delta.dy;


    auto const& surface_info = tools->info_for(surface);

    surface_info.constrain_resize(surface, new_pos, new_size, left_resize, top_resize, bounds);

    apply_resize(surface, surface_info.titlebar, new_pos, new_size);

    return true;
}

void me::CanonicalSurfaceInfoCopy::constrain_resize(
    std::shared_ptr<ms::Surface> const& surface,
    Point& requested_pos,
    Size& requested_size,
    bool const left_resize,
    bool const top_resize,
    Rectangle const& /*bounds*/) const
{
    Point new_pos = requested_pos;
    Size new_size = requested_size;

    if (min_aspect.is_set())
    {
        auto const ar = min_aspect.value();

        auto const error = new_size.height.as_int()*long(ar.width) - new_size.width.as_int()*long(ar.height);

        if (error > 0)
        {
            // Add (denominator-1) to numerator to ensure rounding up
            auto const width_correction  = (error+(ar.height-1))/ar.height;
            auto const height_correction = (error+(ar.width-1))/ar.width;

            if (width_correction < height_correction)
            {
                new_size.width = new_size.width + DeltaX(width_correction);
            }
            else
            {
                new_size.height = new_size.height - DeltaY(height_correction);
            }
        }
    }

    if (max_aspect.is_set())
    {
        auto const ar = max_aspect.value();

        auto const error = new_size.width.as_int()*long(ar.height) - new_size.height.as_int()*long(ar.width);

        if (error > 0)
        {
            // Add (denominator-1) to numerator to ensure rounding up
            auto const height_correction = (error+(ar.width-1))/ar.width;
            auto const width_correction  = (error+(ar.height-1))/ar.height;

            if (width_correction < height_correction)
            {
                new_size.width = new_size.width - DeltaX(width_correction);
            }
            else
            {
                new_size.height = new_size.height + DeltaY(height_correction);
            }
        }
    }

    if (min_width > new_size.width)
        new_size.width = min_width;

    if (min_height > new_size.height)
        new_size.height = min_height;

    if (max_width < new_size.width)
        new_size.width = max_width;

    if (max_height < new_size.height)
        new_size.height = max_height;

    if (width_inc.is_set())
    {
        auto const width = new_size.width.as_int() - min_width.as_int();
        auto inc = width_inc.value().as_int();
        if (width % inc)
            new_size.width = min_width + DeltaX{inc*(((2L*width + inc)/2)/inc)};
    }

    if (height_inc.is_set())
    {
        auto const height = new_size.height.as_int() - min_height.as_int();
        auto inc = height_inc.value().as_int();
        if (height % inc)
            new_size.height = min_height + DeltaY{inc*(((2L*height + inc)/2)/inc)};
    }

    if (left_resize)
        new_pos.x += new_size.width - requested_size.width;

    if (top_resize)
        new_pos.y += new_size.height - requested_size.height;

    // placeholder - constrain onscreen

    switch (state)
    {
    case mir_surface_state_restored:
        break;

    // "A vertically maximised surface is anchored to the top and bottom of
    // the available workspace and can have any width."
    case mir_surface_state_vertmaximized:
        new_pos.y = surface->top_left().y;
        new_size.height = surface->size().height;
        break;

    // "A horizontally maximised surface is anchored to the left and right of
    // the available workspace and can have any height"
    case mir_surface_state_horizmaximized:
        new_pos.x = surface->top_left().x;
        new_size.width = surface->size().width;
        break;

    // "A maximised surface is anchored to the top, bottom, left and right of the
    // available workspace. For example, if the launcher is always-visible then
    // the left-edge of the surface is anchored to the right-edge of the launcher."
    case mir_surface_state_maximized:
    default:
        new_pos.x = surface->top_left().x;
        new_pos.y = surface->top_left().y;
        new_size.width = surface->size().width;
        new_size.height = surface->size().height;
        break;
    }

    requested_pos = new_pos;
    requested_size = new_size;
}

void me::CanonicalWindowManagerPolicyCopy::apply_resize(
    std::shared_ptr<ms::Surface> const& surface,
    std::shared_ptr<ms::Surface> const& titlebar,
    Point const& new_pos,
    Size const& new_size) const
{
    if (titlebar)
        titlebar->resize({new_size.width, Height{title_bar_height}});

    surface->resize(new_size);

    move_tree(surface, new_pos-surface->top_left());
}

bool me::CanonicalWindowManagerPolicyCopy::drag(std::shared_ptr<ms::Surface> surface, Point to, Point from, Rectangle /*bounds*/)
{
    if (!surface)
        return false;

    if (!surface->input_area_contains(from) && !tools->info_for(surface).titlebar)
        return false;

    auto movement = to - from;

    // placeholder - constrain onscreen

    switch (tools->info_for(surface).state)
    {
    case mir_surface_state_restored:
        break;

    // "A vertically maximised surface is anchored to the top and bottom of
    // the available workspace and can have any width."
    case mir_surface_state_vertmaximized:
        movement.dy = DeltaY(0);
        break;

    // "A horizontally maximised surface is anchored to the left and right of
    // the available workspace and can have any height"
    case mir_surface_state_horizmaximized:
        movement.dx = DeltaX(0);
        break;

    // "A maximised surface is anchored to the top, bottom, left and right of the
    // available workspace. For example, if the launcher is always-visible then
    // the left-edge of the surface is anchored to the right-edge of the launcher."
    case mir_surface_state_maximized:
    case mir_surface_state_fullscreen:
    default:
        return true;
    }

    move_tree(surface, movement);

    return true;
}

void me::CanonicalWindowManagerPolicyCopy::move_tree(std::shared_ptr<ms::Surface> const& root, Displacement movement) const
{
    root->move_to(root->top_left() + movement);

    for (auto const& child: tools->info_for(root).children)
    {
        move_tree(child.lock(), movement);
    }
}

void me::CanonicalWindowManagerPolicyCopy::raise_tree(std::shared_ptr<scene::Surface> const& root) const
{
    SurfaceSet surfaces;
    std::function<void(std::weak_ptr<scene::Surface> const& surface)> const add_children =
        [&,this](std::weak_ptr<scene::Surface> const& surface)
        {
            auto const& info_for = tools->info_for(surface);
            surfaces.insert(begin(info_for.children), end(info_for.children));
            for (auto const& child : info_for.children)
                add_children(child);
        };

    surfaces.insert(root);
    add_children(root);

    tools->raise(surfaces);
}
