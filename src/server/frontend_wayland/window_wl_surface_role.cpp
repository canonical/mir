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

#include "window_wl_surface_role.h"

#include "output_manager.h"
#include "wayland_utils.h"
#include "wl_surface.h"
#include "wayland_surface_observer.h"
#include "wl_seat.h"
#include "null_event_sink.h"

#include "mir/frontend/wayland.h"
#include "mir/wayland/client.h"
#include "mir/shell/surface_specification.h"
#include "mir/shell/shell.h"
#include "mir/scene/surface.h"
#include "mir/events/input_event.h"
#include "mir/log.h"

#include <boost/throw_exception.hpp>

#include <algorithm>

namespace mf = mir::frontend;
namespace mw = mir::wayland;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
geom::Size const max_possible_size{
    std::numeric_limits<int>::max(),
    std::numeric_limits<int>::max()};

/// Clears pending if it holds a value different than cache
/// sets cache to pending and leaves pending alone if it holds a different value
template<typename T>
inline void clear_pending_if_unchanged(mir::optional_value<T>& pending, T& cache)
{
    if (pending)
    {
        if (pending.value() == cache)
            pending.consume();
        else
            cache = pending.value();
    }
}
}

mf::WindowWlSurfaceRole::WindowWlSurfaceRole(
    Executor& wayland_executor,
    WlSeat* seat,
    mw::Client* client,
    WlSurface* surface,
    std::shared_ptr<msh::Shell> const& shell,
    OutputManager* output_manager)
    : surface{surface},
      client{client},
      shell{shell},
      session{client->client_session()},
      output_manager{output_manager},
      wayland_executor{wayland_executor},
      observer{std::make_shared<WaylandSurfaceObserver>(wayland_executor, seat, surface, this)},
      committed_min_size{0, 0},
      committed_max_size{max_possible_size}
{
    spec().type = mir_window_type_freestyle;
    surface->set_role(this);
    output_manager->add_listener(this);
}

mf::WindowWlSurfaceRole::~WindowWlSurfaceRole()
{
    output_manager->current_config().for_each_output(
        [&](graphics::DisplayConfigurationOutput const& config)
        {
            if (auto* global{output_manager->output_for(config.id).value_or(nullptr)})
            {
                global->remove_listener(this);
            }

        }
    );
    output_manager->remove_listener(this);
    mark_destroyed();
    if (surface)
    {
        surface.value().clear_role();
    }
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell->destroy_surface(session, scene_surface);
        weak_scene_surface.reset();
    }
}

auto mf::WindowWlSurfaceRole::scene_surface() const -> std::optional<std::shared_ptr<scene::Surface>>
{
    auto shared = weak_scene_surface.lock();
    if (shared)
        return shared;
    else
        return std::nullopt;
}

void mf::WindowWlSurfaceRole::populate_spec_with_surface_data(shell::SurfaceSpecification& spec)
{
    spec.streams = std::vector<shell::StreamSpecification>();
    spec.input_shape = std::vector<geom::Rectangle>();
    if (surface)
    {
        surface.value().populate_surface_data(spec.streams.value(), spec.input_shape.value(), {});
    }
}

void mf::WindowWlSurfaceRole::refresh_surface_data_now()
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell::SurfaceSpecification surface_data_spec;
        populate_spec_with_surface_data(surface_data_spec);
        shell->modify_surface(session, scene_surface, surface_data_spec);
    }
}

void mf::WindowWlSurfaceRole::apply_spec(mir::shell::SurfaceSpecification const& new_spec)
{
    if (new_spec.width.is_set())
        pending_explicit_width = new_spec.width.value();
    if (new_spec.height.is_set())
        pending_explicit_height = new_spec.height.value();

    spec().update_from(new_spec);
}

void mf::WindowWlSurfaceRole::set_pending_offset(std::optional<geom::Displacement> const& offset)
{
    if (surface)
    {
        surface.value().set_pending_offset(offset);
    }
}

void mf::WindowWlSurfaceRole::set_pending_width(std::optional<geometry::Width> const& width)
{
    pending_explicit_width = width;
}

void mf::WindowWlSurfaceRole::set_pending_height(std::optional<geometry::Height> const& height)
{
    pending_explicit_height = height;
}

void mf::WindowWlSurfaceRole::set_title(std::string const& title)
{
    spec().name = title;
}

void mf::WindowWlSurfaceRole::set_application_id(std::string const& application_id)
{
    spec().application_id = application_id;
}

void mf::WindowWlSurfaceRole::initiate_interactive_move(uint32_t serial)
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        if (auto const ev = input_event_for(serial))
        {
            shell->request_move(session, scene_surface, mir_event_get_input_event(ev.get()));
        }
    }
}

void mf::WindowWlSurfaceRole::initiate_interactive_resize(MirResizeEdge edge, uint32_t serial)
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        if (auto const ev = input_event_for(serial))
        {
            shell->request_resize(session, scene_surface, mir_event_get_input_event(ev.get()), edge);
        }
    }
}

void mf::WindowWlSurfaceRole::set_parent(std::optional<std::shared_ptr<scene::Surface>> const& parent)
{
    auto& mods = spec();
    if (parent)
    {
        mods.parent = parent.value();
    }
    else if (mods.parent)
    {
        mods.parent.consume();
    }
}

void mf::WindowWlSurfaceRole::set_max_size(int32_t width, int32_t height)
{
    auto& mods = spec();
    mods.max_width = width ? geom::Width{width} : max_possible_size.width;
    mods.max_height = height ? geom::Height{height} : max_possible_size.height;
}

void mf::WindowWlSurfaceRole::set_min_size(int32_t width, int32_t height)
{
    auto& mods = spec();
    mods.min_width = geom::Width{width};
    mods.min_height = geom::Height{height};
}

void mf::WindowWlSurfaceRole::set_fullscreen(std::optional<struct wl_resource*> const& output)
{
    // We must process this request immediately (i.e. don't defer until commit())
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell::SurfaceSpecification mods;
        mods.state = scene_surface->state_tracker().with(mir_window_state_fullscreen).active_state();
        if (auto const output_global = OutputGlobal::from(output.value_or(nullptr)))
        {
            mods.output_id = output_global->current_config().id;
        }
        shell->modify_surface(session, scene_surface, mods);
    }
    else
    {
        spec().state = mir_window_state_fullscreen;
        if (auto const output_id = OutputManager::output_id_for(output))
        {
            spec().output_id = output_id.value();
        }
        create_scene_surface();
    }
}

void mir::frontend::WindowWlSurfaceRole::set_type(MirWindowType type)
{
    spec().type = type;
}

void mf::WindowWlSurfaceRole::add_state_now(MirWindowState state)
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell::SurfaceSpecification mods;
        mods.state = scene_surface->state_tracker().with(state).active_state();
        shell->modify_surface(session, scene_surface, mods);
    }
    else
    {
        spec().state = state;
        create_scene_surface();
    }
}

void mf::WindowWlSurfaceRole::remove_state_now(MirWindowState state)
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        shell::SurfaceSpecification mods;
        mods.state = scene_surface->state_tracker().without(state).active_state();
        shell->modify_surface(session, scene_surface, mods);
    }
    else
    {
        create_scene_surface();
    }
}

auto mf::WindowWlSurfaceRole::pending_size() const -> geom::Size
{
    auto size = current_size();
    if (pending_explicit_width)
        size.width = pending_explicit_width.value();
    if (pending_explicit_height)
        size.height = pending_explicit_height.value();
    return size;
}

auto mf::WindowWlSurfaceRole::current_size() const -> geom::Size
{
    auto size = geom::Size{640, 480};
    if ((!committed_width_set_explicitly || !committed_height_set_explicitly) && surface)
    {
        if (auto const buffer_size = surface.value().buffer_size())
        {
            if (!committed_width_set_explicitly)
            {
                size.width = buffer_size->width;
            }
            if (!committed_height_set_explicitly)
            {
                size.height = buffer_size->height;
            }
        }
    }
    return size;
}

auto mf::WindowWlSurfaceRole::requested_window_size() const -> std::optional<geom::Size>
{
    return observer->requested_window_size();
}

auto mf::WindowWlSurfaceRole::window_state() const -> MirWindowState
{
    return observer->state();
}

auto mf::WindowWlSurfaceRole::is_active() const -> bool
{
    if (auto const scene_surface = weak_scene_surface.lock())
    {
        auto const state = scene_surface->focus_state();
        return state != mir_window_focus_state_unfocused;
    }
    else
    {
        return false;
    }
}

void mf::WindowWlSurfaceRole::commit(WlSurfaceState const& state)
{
    if (!surface)
    {
        return;
    }

    surface.value().commit(state);
    handle_commit();

    auto size = pending_size();
    observer->latest_client_size(size);

    if (auto const scene_surface = weak_scene_surface.lock())
    {
        bool const is_mapped = scene_surface->visible();
        bool const should_be_mapped = static_cast<bool>(surface.value().buffer_size());
        if (!is_mapped && should_be_mapped && scene_surface->state() == mir_window_state_hidden)
        {
            spec().state = mir_window_state_restored;
        }
        else if (is_mapped && !should_be_mapped)
        {
            spec().state = mir_window_state_hidden;
        }

        apply_client_size(spec());

        if (state.surface_data_needs_refresh())
        {
            populate_spec_with_surface_data(spec());
        }

        if (pending_changes)
        {
            clear_pending_if_unchanged(pending_changes->min_width,  committed_min_size.width);
            clear_pending_if_unchanged(pending_changes->min_height, committed_min_size.height);
            clear_pending_if_unchanged(pending_changes->max_width,  committed_max_size.width);
            clear_pending_if_unchanged(pending_changes->max_height, committed_max_size.height);
        }

        if (pending_changes && !pending_changes->is_empty())
            shell->modify_surface(session, scene_surface, *pending_changes);

        pending_changes.reset();
    }
    else
    {
        create_scene_surface();
    }

    if (pending_explicit_width)
        committed_width_set_explicitly = true;
    if (pending_explicit_height)
        committed_height_set_explicitly = true;
    pending_explicit_width = std::nullopt;
    pending_explicit_height = std::nullopt;

    if (!scene_surface_marked_ready && surface.value().buffer_size())
    {
        if (auto const scene_surface = weak_scene_surface.lock())
        {
            shell->surface_ready(scene_surface);
            scene_surface_marked_ready = true;
        }
    }
}

void mf::WindowWlSurfaceRole::surface_destroyed()
{
    if (!client->is_being_destroyed())
    {
        mark_destroyed();

        // "When a client wants to destroy a wl_surface, they must destroy this 'role object' before the wl_surface"
        // NOTE: the wl_shell_surface specification seems contradictory, so this method is overridden in its implementation
        // NOTE: it's also overridden in layer shell, for reasons explained there
        log_warning("wl_surface@%s destroyed before associated role",
                    (surface ? std::to_string(wl_resource_get_id(surface.value().resource)) : "?").c_str());

        // This isn't strictly correct (as it only applies to wl-shell) but is commonly assumed (e.g. by SDL2) and
        // implemented (e.g. Mutter for xdg-shell)
        // wl_shell_surface: "On the server side the object is automatically destroyed when the related wl_surface is
        // destroyed"
        destroy_role();
    }
    else
    {
        // If the client has been destroyed, everything is getting cleaned up in an arbitrary order. Delete this so our
        // derived class doesn't end up using the now-defunct surface.
        delete this;
    }
}

auto mf::WindowWlSurfaceRole::input_event_for(uint32_t serial) -> std::shared_ptr<MirInputEvent const>
{
    auto const ev = client->event_for(serial);
    if (ev && ev.value() && mir_event_get_type(ev.value().get()) == mir_event_type_input)
    {
        return std::dynamic_pointer_cast<MirInputEvent const>(ev.value());
    }
    else
    {
        return {};
    }
}

void mf::WindowWlSurfaceRole::output_global_created(OutputGlobal *global)
{
    global->add_listener(this);
}

auto mf::WindowWlSurfaceRole::output_config_changed(graphics::DisplayConfigurationOutput const& config) -> bool
{
    if (surface)
    {
        if (auto id_it{std::ranges::find(pending_enter_events, config.id)}; id_it != pending_enter_events.end())
        {
            if (auto* global{output_manager->output_for(config.id).value_or(nullptr)})
            {
                global->for_each_output_bound_by(
                    client,
                    [&](OutputInstance* instance)
                    {
                        surface.value().send_enter_event(instance->resource);
                        pending_enter_events.erase(id_it);
                    });
            }
        }
    }

    return true;
}

mir::shell::SurfaceSpecification& mf::WindowWlSurfaceRole::spec()
{
    if (!pending_changes)
        pending_changes = std::make_unique<mir::shell::SurfaceSpecification>();

    return *pending_changes;
}

void mf::WindowWlSurfaceRole::create_scene_surface()
{
    if (weak_scene_surface.lock() || !surface)
        return;

    auto& mods = spec();

    apply_client_size(mods);

    mods.streams = std::vector<shell::StreamSpecification>{};
    mods.input_shape = std::vector<geom::Rectangle>{};
    surface.value().populate_surface_data(mods.streams.value(), mods.input_shape.value(), {});

    auto const scene_surface = shell->create_surface(session, surface, mods, observer, &wayland_executor);
    weak_scene_surface = scene_surface;

    if (mods.min_width)  committed_min_size.width  = mods.min_width.value();
    if (mods.min_height) committed_min_size.height = mods.min_height.value();
    if (mods.max_width)  committed_max_size.width  = mods.max_width.value();
    if (mods.max_height) committed_max_size.height = mods.max_height.value();

    // The shell isn't guaranteed to respect the requested size
    // TODO: make initial updates atomic somehow
    auto const content_size = scene_surface->content_size();
    observer->content_resized_to(scene_surface.get(), content_size);
    auto const focus_state = scene_surface->focus_state();
    if (focus_state != mir_window_focus_state_unfocused)
    {
        observer->attrib_changed(scene_surface.get(), mir_window_attrib_focus, focus_state);
    }

    // Invalidates mods
    pending_changes.reset();
}

void mf::WindowWlSurfaceRole::handle_enter_output(graphics::DisplayConfigurationOutputId id)
{
    bool event_sent{};
    if (surface)
    {
        if (auto* global{output_manager->output_for(id).value_or(nullptr)})
        {
            auto const& config{global->current_config()};
            if (config.id == id)
            {
                global->for_each_output_bound_by(
                    client,
                    [&](OutputInstance* instance)
                    {
                        surface.value().send_enter_event(instance->resource);
                        event_sent = true;
                    });
            }
        }
    }
    if (!event_sent)
    {
        pending_enter_events.push_back(id);
    }
}

void mf::WindowWlSurfaceRole::handle_leave_output(graphics::DisplayConfigurationOutputId id) const
{
    if (surface)
    {
        if (auto* global{output_manager->output_for(id).value_or(nullptr)})
        {
            auto const& config{global->current_config()};
            if (config.id == id)
            {
                global->for_each_output_bound_by(
                    client,
                    [&](OutputInstance* instance)
                    {
                        surface.value().send_leave_event(instance->resource);
                    });
            }
        }
    }
}

void mf::WindowWlSurfaceRole::apply_client_size(mir::shell::SurfaceSpecification& mods)
{
    if ((!committed_width_set_explicitly || !committed_height_set_explicitly) && surface)
    {
        if (auto const buffer_size = surface.value().buffer_size())
        {
            if (!committed_width_set_explicitly)
            {
                mods.width = buffer_size->width;
            }
            if (!committed_height_set_explicitly)
            {
                mods.height = buffer_size->height;
            }
        }
    }

    if (pending_explicit_width)
        mods.width = pending_explicit_width.value();
    if (pending_explicit_height)
        mods.height = pending_explicit_height.value();
}
