/*
 * Copyright © 2015 Canonical Ltd.
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

#include "mir/shell/abstract_shell.h"
#include "mir/shell/input_targeter.h"
#include "mir/shell/shell_report.h"
#include "mir/shell/surface_specification.h"
#include "mir/shell/surface_stack.h"
#include "mir/shell/window_manager.h"
#include "mir/scene/prompt_session.h"
#include "mir/scene/prompt_session_manager.h"
#include "mir/scene/null_surface_observer.h"
#include "mir/scene/session_coordinator.h"
#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"
#include "mir/input/seat.h"
#include "decoration/manager.h"

#include <algorithm>
#include <vector>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{

struct UpdateConfinementOnSurfaceChanges : ms::NullSurfaceObserver
{
    UpdateConfinementOnSurfaceChanges(msh::AbstractShell* shell) :
        shell(shell)
    {
    }

    void content_resized_to(ms::Surface const*, geom::Size const& /*content_size*/) override
    {
        update_confinement_region();
    }

    void moved_to(ms::Surface const*, geom::Point const& /*top_left*/) override
    {
        update_confinement_region();
    }

private:
    void update_confinement_region()
    {
        shell->update_focused_surface_confined_region();
    }

    msh::AbstractShell* shell;
};
}

msh::AbstractShell::AbstractShell(
    std::shared_ptr<InputTargeter> const& input_targeter,
    std::shared_ptr<msh::SurfaceStack> const& surface_stack,
    std::shared_ptr<ms::SessionCoordinator> const& session_coordinator,
    std::shared_ptr<ms::PromptSessionManager> const& prompt_session_manager,
    std::shared_ptr<ShellReport> const& report,
    std::function<std::shared_ptr<shell::WindowManager>(FocusController* focus_controller)> const& wm_builder,
    std::shared_ptr<mi::Seat> const& seat,
    std::shared_ptr<decoration::Manager> const& decoration_manager) :
    input_targeter(input_targeter),
    surface_stack(surface_stack),
    session_coordinator(session_coordinator),
    prompt_session_manager(prompt_session_manager),
    window_manager(wm_builder(this)),
    seat(seat),
    report(report),
    focus_surface_observer(std::make_shared<UpdateConfinementOnSurfaceChanges>(this)),
    decoration_manager(decoration_manager)
{
}

msh::AbstractShell::~AbstractShell() noexcept
{
    decoration_manager->undecorate_all();
}

void msh::AbstractShell::update_focused_surface_confined_region()
{
    auto const current_focus = focus_surface.lock();

    if (current_focus && current_focus->confine_pointer_state() == mir_pointer_confined_to_window)
    {
        seat->set_confinement_regions({current_focus->input_bounds()});
    }
}

std::shared_ptr<ms::Session> msh::AbstractShell::open_session(
    pid_t client_pid,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    auto const result = session_coordinator->open_session(client_pid, name, sink);
    window_manager->add_session(result);
    report->opened_session(*result);
    return result;
}

void msh::AbstractShell::close_session(
    std::shared_ptr<ms::Session> const& session)
{
    report->closing_session(*session);
    prompt_session_manager->remove_session(session);

    // TODO revisit this when reworking the AbstractShell/WindowManager interactions
    // this is an ugly kludge to extract the list of surfaces owned by the session
    // We're likely to have this information in the WindowManager implementation
    std::set<std::shared_ptr<ms::Surface>> surfaces;
    for (auto surface = session->default_surface(); surface; surface = session->surface_after(surface))
        if (!surfaces.insert(surface).second) break;

    // this is an ugly kludge to remove the each of the surfaces owned by the session
    // We could likely do this better (and atomically) within the WindowManager
    for (auto const& surface : surfaces)
    {
        decoration_manager->undecorate(surface);
        window_manager->remove_surface(session, surface);
    }

    session_coordinator->close_session(session);
    window_manager->remove_session(session);
}

auto msh::AbstractShell::create_surface(
    std::shared_ptr<ms::Session> const& session,
    ms::SurfaceCreationParameters const& params,
    std::shared_ptr<ms::SurfaceObserver> const& observer) -> std::shared_ptr<ms::Surface>
{
    auto const build = [observer](std::shared_ptr<ms::Session> const& session, ms::SurfaceCreationParameters const& placed_params)
        {
            return session->create_surface(session, placed_params, observer);
        };

    auto const result = window_manager->add_surface(session, params, build);
    report->created_surface(*session, *result);

    if (params.server_side_decorated.is_set() && params.server_side_decorated.value())
    {
        decoration_manager->decorate(result);
    }
    return result;
}

void msh::AbstractShell::modify_surface(std::shared_ptr<scene::Session> const& session, std::shared_ptr<scene::Surface> const& surface, SurfaceSpecification const& modifications)
{
    auto wm_relevant_mods = modifications;

    auto const window_size{surface->window_size()};
    auto const content_size{surface->content_size()};
    auto const horiz_frame_padding = window_size.width - content_size.width;
    auto const vert_frame_padding = window_size.height - content_size.height;
    if (wm_relevant_mods.width.is_set())
        wm_relevant_mods.width.value() += horiz_frame_padding;
    if (wm_relevant_mods.height.is_set())
        wm_relevant_mods.height.value() += vert_frame_padding;
    if (wm_relevant_mods.max_width.is_set())
        wm_relevant_mods.max_width.value() += horiz_frame_padding;
    if (wm_relevant_mods.max_height.is_set())
        wm_relevant_mods.max_height.value() += vert_frame_padding;
    if (wm_relevant_mods.min_width.is_set())
        wm_relevant_mods.min_width.value() += horiz_frame_padding;
    if (wm_relevant_mods.min_height.is_set())
        wm_relevant_mods.min_height.value() += vert_frame_padding;

    if (wm_relevant_mods.aux_rect.is_set())
        wm_relevant_mods.aux_rect.value().top_left += surface->content_offset();

    report->update_surface(*session, *surface, wm_relevant_mods);

    if (wm_relevant_mods.streams.is_set())
    {
        session->configure_streams(*surface, wm_relevant_mods.streams.consume());
    }
    if (!wm_relevant_mods.is_empty())
    {
        window_manager->modify_surface(session, surface, wm_relevant_mods);
    }

    if (modifications.cursor_image.is_set())
    {
        if (modifications.cursor_image.value() == nullptr)
        {
            surface->set_cursor_image({});
        }
        else
        {
            surface->set_cursor_image(modifications.cursor_image.value());
        }
    }

    if (modifications.stream_cursor.is_set())
    {
        auto const& cursor = modifications.stream_cursor.value();
        if (auto const stream = cursor.stream.lock())
        {
            auto hotspot = cursor.hotspot;
            surface->set_cursor_stream(stream, hotspot);
        }
        else
        {
            surface->set_cursor_image({});
        }
    }

    if (modifications.confine_pointer.is_set() && focused_surface() == surface)
    {
        if (surface->confine_pointer_state() == mir_pointer_confined_to_window)
        {
            seat->set_confinement_regions({surface->input_bounds()});
        }
        else
        {
            seat->reset_confinement_regions();
        }
    }
}

void msh::AbstractShell::destroy_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface)
{
    report->destroying_surface(*session, *surface);
    decoration_manager->undecorate(surface);
    window_manager->remove_surface(session, surface);
}

std::shared_ptr<ms::PromptSession> msh::AbstractShell::start_prompt_session_for(
    std::shared_ptr<ms::Session> const& session,
    scene::PromptSessionCreationParameters const& params)
{
    auto const result = prompt_session_manager->start_prompt_session_for(session, params);
    report->started_prompt_session(*result, *session);
    return result;
}

void msh::AbstractShell::add_prompt_provider_for(
    std::shared_ptr<ms::PromptSession> const& prompt_session,
    std::shared_ptr<ms::Session> const& session)
{
    prompt_session_manager->add_prompt_provider(prompt_session, session);
    report->added_prompt_provider(*prompt_session, *session);
}

void msh::AbstractShell::stop_prompt_session(
    std::shared_ptr<ms::PromptSession> const& prompt_session)
{
    report->stopping_prompt_session(*prompt_session);
    prompt_session_manager->stop_prompt_session(prompt_session);
}

int msh::AbstractShell::set_surface_attribute(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    MirWindowAttrib attrib,
    int value)
{
    report->update_surface(*session, *surface, attrib, value);
    return window_manager->set_surface_attribute(session, surface, attrib, value);
}

int msh::AbstractShell::get_surface_attribute(
    std::shared_ptr<ms::Surface> const& surface,
    MirWindowAttrib attrib)
{
    return surface->query(attrib);
}

void msh::AbstractShell::raise_surface(
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface,
    uint64_t timestamp)
{
    window_manager->handle_raise_surface(session, surface, timestamp);
}

void msh::AbstractShell::request_drag_and_drop(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    uint64_t timestamp)
{
    window_manager->handle_request_drag_and_drop(session, surface, timestamp);
}

void msh::AbstractShell::request_move(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    uint64_t timestamp)
{
    window_manager->handle_request_move(session, surface, timestamp);
}

void msh::AbstractShell::request_resize(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    uint64_t timestamp,
    MirResizeEdge edge)
{
    window_manager->handle_request_resize(session, surface, timestamp, edge);
}

void msh::AbstractShell::focus_next_session()
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    auto const focused_session = focus_session.lock();
    auto successor = session_coordinator->successor_of(focused_session);
    auto const sentinel = successor;

    while (successor != nullptr &&
           successor->default_surface() == nullptr)
    {
        successor = session_coordinator->successor_of(successor);
        if (successor == sentinel)
            break;
    }

    auto const surface = successor ? successor->default_surface() : nullptr;

    if (!surface)
        successor = nullptr;

    update_focus_locked(lock, successor, surface);
}

void msh::AbstractShell::focus_prev_session()
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    auto const focused_session = focus_session.lock();
    auto predecessor = session_coordinator->predecessor_of(focused_session);
    auto const sentinel = predecessor;

    while (predecessor != nullptr &&
           predecessor->default_surface() == nullptr)
    {
        predecessor = session_coordinator->predecessor_of(predecessor);
        if (predecessor == sentinel)
            break;
    }

    auto const surface = predecessor ? predecessor->default_surface() : nullptr;

    if (!surface)
        predecessor = nullptr;

    update_focus_locked(lock, predecessor, surface);
}

std::shared_ptr<ms::Session> msh::AbstractShell::focused_session() const
{
    std::unique_lock<std::mutex> lg(focus_mutex);
    return focus_session.lock();
}

std::shared_ptr<ms::Surface> msh::AbstractShell::focused_surface() const
{
    std::unique_lock<std::mutex> lock(focus_mutex);
    return focus_surface.lock();
}

void msh::AbstractShell::set_focus_to(
    std::shared_ptr<ms::Session> const& focus_session,
    std::shared_ptr<ms::Surface> const& focus_surface)
{
    std::unique_lock<std::mutex> lock(focus_mutex);

    notify_focus_locked(lock, focus_session, focus_surface);
    update_focus_locked(lock, focus_session, focus_surface);
}

void msh::AbstractShell::notify_focus_locked(
    std::unique_lock<std::mutex> const& /*lock*/,
    std::shared_ptr<ms::Session> const& /*session*/,
    std::shared_ptr<ms::Surface> const& surface)
{
    auto const current_focus = notified_focus_surface.lock();

    std::vector<std::shared_ptr<ms::Surface>> new_focus_tree;

    for (auto item = surface; item; item = item->parent())
    {
        new_focus_tree.insert(begin(new_focus_tree), item);
    }

    std::vector<std::shared_ptr<ms::Surface>> current_focus_tree;

    for (auto item = current_focus; item; item = item->parent())
    {
        current_focus_tree.push_back(item);
    }

    if (surface != current_focus)
    {
        notified_focus_surface = surface;
        seat->reset_confinement_regions();

        for (auto const& item : current_focus_tree)
        {
            // It looks odd to unfocus `surface` here and focus it later, but this is a workaround
            // for bug 823: There seems to be no easy alternative to prod the surface event source.
            // TODO: find a better way.
            if (find(begin(new_focus_tree), end(new_focus_tree), item) == end(new_focus_tree) ||
                item == surface)
            {
                item->set_focus_state(mir_window_focus_state_unfocused);
            }
        }

        if (current_focus)
        {
            current_focus->remove_observer(focus_surface_observer);
        }

        if (surface)
        {
            if (surface->confine_pointer_state() == mir_pointer_confined_to_window)
            {
                seat->set_confinement_regions({surface->input_bounds()});
            }

            // Ensure the surface has really taken the focus before notifying it that it is focused
            input_targeter->set_focus(surface);
            surface->consume(seat->create_device_state().get());
            surface->add_observer(focus_surface_observer);

            for (auto const& item : new_focus_tree)
            {
                if (find(begin(current_focus_tree), end(current_focus_tree), item) == end(current_focus_tree) ||
                    item == surface)
                {
                    item->set_focus_state(mir_window_focus_state_focused);
                }
            }
        }
        else
        {
            input_targeter->clear_focus();
        }
    }
}

void msh::AbstractShell::update_focus_locked(
    std::unique_lock<std::mutex> const& /*lock*/,
    std::shared_ptr<ms::Session> const& session,
    std::shared_ptr<ms::Surface> const& surface)
{
    auto const current_session = focus_session.lock();

    if (session != current_session)
    {
        focus_session = session;

        if (session)
        {
            session_coordinator->set_focus_to(session);
        }
        else
        {
            session_coordinator->unset_focus();
        }
    }
    focus_surface = surface;
    report->input_focus_set_to(session.get(), surface.get());
}

void msh::AbstractShell::add_display(geometry::Rectangle const& area)
{
    report->adding_display(area);
    window_manager->add_display(area);
}

void msh::AbstractShell::remove_display(geometry::Rectangle const& area)
{
    report->removing_display(area);
    window_manager->remove_display(area);
}

bool msh::AbstractShell::handle(MirEvent const& event)
{
    if (mir_event_get_type(&event) != mir_event_type_input)
        return false;

    auto const input_event = mir_event_get_input_event(&event);

    switch (mir_input_event_get_type(input_event))
    {
    case mir_input_event_type_key:
        return window_manager->handle_keyboard_event(mir_input_event_get_keyboard_event(input_event));

    case mir_input_event_type_touch:
        return window_manager->handle_touch_event(mir_input_event_get_touch_event(input_event));

    case mir_input_event_type_pointer:
        return window_manager->handle_pointer_event(mir_input_event_get_pointer_event(input_event));
    
    case mir_input_event_types:
        abort();
        return false;
    }

    return false;
}

auto msh::AbstractShell::surface_at(geometry::Point cursor) const
-> std::shared_ptr<scene::Surface>
{
    return surface_stack->surface_at(cursor);
}

void msh::AbstractShell::raise(SurfaceSet const& surfaces)
{
    surface_stack->raise(surfaces);
    report->surfaces_raised(surfaces);
}

void msh::AbstractShell::set_drag_and_drop_handle(std::vector<uint8_t> const& handle)
{
    input_targeter->set_drag_and_drop_handle(handle);
}

void msh::AbstractShell::clear_drag_and_drop_handle()
{
    input_targeter->clear_drag_and_drop_handle();
}
