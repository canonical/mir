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
#include "mir/input/seat.h"
#include "mir/wayland/weak.h"
#include "mir/shell/decoration/manager.h"

#include <algorithm>
#include <iterator>
#include <vector>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace mi = mir::input;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
auto get_toplevel(std::shared_ptr<ms::Surface> surface) -> std::shared_ptr<ms::Surface>
{
    std::shared_ptr<ms::Surface> prev;
    while (surface)
    {
        prev = std::move(surface);
        surface = prev->parent();
    }
    return prev;
}

auto get_non_popup_parent(std::shared_ptr<ms::Surface> surface) -> std::shared_ptr<ms::Surface>
{
    while (surface)
    {
        auto const type = surface->type();
        if (type != mir_window_type_gloss &&
            type != mir_window_type_tip &&
            type != mir_window_type_menu)
        {
            break;
        }
        surface = surface->parent();
    }
    return surface;
}

// Returns a vector comprising the supplied surface (if not null), parent, grandparent, etc. in order
auto get_ancestry(std::shared_ptr<ms::Surface> surface) -> std::vector<std::shared_ptr<ms::Surface>>
{
    std::vector<std::shared_ptr<ms::Surface>> result;
    for (auto item = std::move(surface); item; item = item->parent())
    {
        result.push_back(item);
    }
    return result;
}
}

class msh::AbstractShell::SurfaceConfinementUpdater : public scene::NullSurfaceObserver
{
public:
    SurfaceConfinementUpdater(input::Seat* const seat)
        : seat{seat}
    {
    }

    void set_focus_surface(std::shared_ptr<scene::Surface> const& surface)
    {
        std::lock_guard lock{mutex};
        focus_surface = surface;
    }

private:
    void content_resized_to(scene::Surface const*, geom::Size const& /*content_size*/) override
    {
        update_confinement_region();
    }

    void moved_to(scene::Surface const*, geom::Point const& /*top_left*/) override
    {
        update_confinement_region();
    }

    void update_confinement_region()
    {
        std::lock_guard lock{mutex};

        if (auto const current_focus = focus_surface.lock())
        {
            switch (current_focus->confine_pointer_state())
            {
            case mir_pointer_confined_oneshot:
            case mir_pointer_confined_persistent:
                seat->set_confinement_regions({current_focus->input_bounds()});
                break;

            default:
                break;
            }
        }
    }

    input::Seat* const seat; ///< Shared ownership is held by the shell that owns us
    std::mutex mutex;
    std::weak_ptr<scene::Surface> focus_surface;
};

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
    surface_confinement_updater(std::make_shared<SurfaceConfinementUpdater>(seat.get())),
    decoration_manager(decoration_manager)
{
}

msh::AbstractShell::~AbstractShell() noexcept
{
    decoration_manager->undecorate_all();
    if (auto const current_keyboard_focus = notified_keyboard_focus_surface.lock())
    {
        current_keyboard_focus->unregister_interest(*surface_confinement_updater);
    }
}

std::shared_ptr<ms::Session> msh::AbstractShell::open_session(
    pid_t client_pid,
    Fd socket_fd,
    std::string const& name,
    std::shared_ptr<mf::EventSink> const& sink)
{
    auto const result = session_coordinator->open_session(client_pid, socket_fd, name, sink);
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

    // this is an ugly kludge to remove each of the surfaces owned by the session
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
    wayland::Weak<frontend::WlSurface> const& wayland_surface,
    SurfaceSpecification const& spec,
    std::shared_ptr<ms::SurfaceObserver> const& observer,
    Executor* observer_executor) -> std::shared_ptr<ms::Surface>
{
    // Instead of a shared pointer, a local variable could be used and the lambda could capture a reference to it
    // This should be safe, but could be the source of nasty bugs and crashes if the wm did something unexpected
    auto const should_decorate = std::make_shared<bool>(false);
    auto const build = [observer, observer_executor, should_decorate, wayland_surface](
            std::shared_ptr<ms::Session> const& session,
            msh::SurfaceSpecification const& placed_params)
        {
            if (placed_params.server_side_decorated.is_set() && placed_params.server_side_decorated.value())
            {
                *should_decorate = true;
            }
            return session->create_surface(session, wayland_surface, placed_params, observer, observer_executor);
        };

    auto const result = window_manager->add_surface(session, spec, build);
    report->created_surface(*session, *result);

    if (*should_decorate)
    {
        decoration_manager->decorate(result);
    }
    return result;
}

void msh::AbstractShell::surface_ready(std::shared_ptr<ms::Surface> const& surface)
{
    window_manager->surface_ready(surface);
    if (surface->type() == mir_window_type_menu)
    {
        std::lock_guard lock(focus_mutex);
        add_grabbing_popup(surface);
    }
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

    if (modifications.confine_pointer.is_set())
    {
        auto const prev_state = surface->confine_pointer_state();
        auto const new_state = modifications.confine_pointer.value();

        if ((prev_state != mir_pointer_unconfined) && (new_state != mir_pointer_unconfined))
        {
            // TODO need to report "already_constrained"
        }

        surface->set_confine_pointer_state(new_state);

        if (focused_surface() == surface)
        {
            update_confinement_for(surface);
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

void msh::AbstractShell::request_move(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    MirInputEvent const* event)
{
    window_manager->handle_request_move(session, surface, event);
}

void msh::AbstractShell::request_resize(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<scene::Surface> const& surface,
    MirInputEvent const* event,
    MirResizeEdge edge)
{
    window_manager->handle_request_resize(session, surface, event, edge);
}

void msh::AbstractShell::focus_next_session()
{
    std::unique_lock lock(focus_mutex);
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
    std::unique_lock lock(focus_mutex);
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
    std::unique_lock lg(focus_mutex);
    return focus_session.lock();
}

std::shared_ptr<ms::Surface> msh::AbstractShell::focused_surface() const
{
    std::unique_lock lock(focus_mutex);
    return focus_surface.lock();
}

void msh::AbstractShell::set_popup_grab_tree(std::shared_ptr<scene::Surface> const& surface)
{
    std::lock_guard lock(focus_mutex);
    set_popup_parent(get_toplevel(surface));
}

void msh::AbstractShell::set_focus_to(
    std::shared_ptr<ms::Session> const& focus_session,
    std::shared_ptr<ms::Surface> const& focus_surface)
{
    std::unique_lock lock(focus_mutex);

    if (last_requested_focus_surface.lock() != focus_surface)
    {
        last_requested_focus_surface = focus_surface;
        auto new_active_surfaces = get_ancestry(focus_surface);

        /// HACK: Grabbing popups (menus, in Mir terminology) should be given keyboard focus according to xdg-shell,
        /// however, giving menus keyboard focus breaks Qt submenus. As of February 2022 Weston and other compositors
        /// disobey the protocol by not giving keyboard focus to grabbing popups, so we do the same thing. The behavior
        /// is subject change. See: https://github.com/MirServer/mir/issues/2324.
        auto const new_keyboard_focus_surface = get_non_popup_parent(focus_surface);

        notify_active_surfaces(lock, new_keyboard_focus_surface, std::move(new_active_surfaces));
        set_keyboard_focus_surface(lock, new_keyboard_focus_surface);
    }

    update_focus_locked(lock, focus_session, focus_surface);
}

void msh::AbstractShell::notify_active_surfaces(
    std::unique_lock<std::mutex> const&,
    std::shared_ptr<ms::Surface> const& new_keyboard_focus_surface,
    std::vector<std::shared_ptr<ms::Surface>> new_active_surfaces)
{
    SurfaceSet new_active_set{begin(new_active_surfaces), end(new_active_surfaces)};

    for (auto const& current_active_weak: notified_active_surfaces)
    {
        if (auto const current_active = current_active_weak.lock())
        {
            // If this surface is not still active
            if (new_active_set.find(current_active_weak) == end(new_active_set))
            {
                // If a surface that was previously active is not in the set of new active surfaces, notify it
                current_active->set_focus_state(mir_window_focus_state_unfocused);
            }
        }
    }

    for (auto const& new_active: new_active_surfaces)
    {
        if (new_active != new_keyboard_focus_surface)
        {
            new_active->set_focus_state(mir_window_focus_state_active);
        }
    }

    if (new_keyboard_focus_surface)
    {
        new_keyboard_focus_surface->set_focus_state(mir_window_focus_state_focused);
    }

    notified_active_surfaces = {begin(new_active_surfaces), end(new_active_surfaces)};
}

void msh::AbstractShell::set_keyboard_focus_surface(
    std::unique_lock<std::mutex> const&,
    std::shared_ptr<ms::Surface> const& new_keyboard_focus_surface)
{
    auto const current_keyboard_focus = notified_keyboard_focus_surface.lock();
    if (current_keyboard_focus == new_keyboard_focus_surface)
    {
        return;
    }

    seat->reset_confinement_regions();

    if (current_keyboard_focus)
    {
        current_keyboard_focus->unregister_interest(*surface_confinement_updater);

        switch (current_keyboard_focus->confine_pointer_state())
        {
        case mir_pointer_confined_oneshot:
        case mir_pointer_locked_oneshot:
            current_keyboard_focus->set_confine_pointer_state(mir_pointer_unconfined);
            break;

        default:
            break;
        }
    }

    if (new_keyboard_focus_surface)
    {
        update_confinement_for(new_keyboard_focus_surface);

        input_targeter->set_focus(new_keyboard_focus_surface);
        new_keyboard_focus_surface->consume(seat->create_device_state());
        surface_confinement_updater->set_focus_surface(new_keyboard_focus_surface);
        new_keyboard_focus_surface->register_interest(surface_confinement_updater);
    }
    else
    {
        input_targeter->clear_focus();
    }

    notified_keyboard_focus_surface = new_keyboard_focus_surface;
}

void msh::AbstractShell::update_confinement_for(std::shared_ptr<ms::Surface> const& surface) const
{
    switch (surface->confine_pointer_state())
    {
    case mir_pointer_locked_oneshot:
    case mir_pointer_locked_persistent:
    {
        auto rectangle = surface->input_bounds();
        rectangle.top_left = rectangle.top_left + as_displacement(0.5*rectangle.size);
        rectangle.size = {1,1};
        this->seat->set_confinement_regions({rectangle});
        break;
    }
    case mir_pointer_confined_oneshot:
    case mir_pointer_confined_persistent:
        this->seat->set_confinement_regions({surface->input_bounds()});
        break;

    case mir_pointer_unconfined:
        this->seat->reset_confinement_regions();
        break;
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

void msh::AbstractShell::set_popup_parent(std::shared_ptr<scene::Surface> const& new_popup_parent)
{
    if (new_popup_parent == popup_parent.lock())
    {
        return;
    }
    popup_parent = new_popup_parent;
    for (auto it = grabbing_popups.rbegin(); it != grabbing_popups.rend(); it++)
    {
        if (auto const popup = it->lock())
        {
            popup->request_client_surface_close();
            popup->hide();
        }
    }
}

void msh::AbstractShell::add_grabbing_popup(std::shared_ptr<scene::Surface> const& popup)
{
    if (get_toplevel(popup) == popup_parent.lock())
    {
        grabbing_popups.push_back(popup);
    }
    else
    {
        popup->request_client_surface_close();
        popup->hide();
    }
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

    case mir_input_event_type_keyboard_resync:
        return false;

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

void msh::AbstractShell::swap_z_order(const mir::shell::SurfaceSet &first, const mir::shell::SurfaceSet &second)
{
    surface_stack->swap_z_order(first, second);
}

void msh::AbstractShell::send_to_back(const mir::shell::SurfaceSet &surfaces)
{
    surface_stack->send_to_back(surfaces);
}
