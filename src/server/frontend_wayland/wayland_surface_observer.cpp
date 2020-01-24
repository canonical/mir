/*
 * Copyright © 2018-2019 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 *              William Wold <william.wold@canonical.com>
 */

#include "wayland_surface_observer.h"
#include "wl_seat.h"
#include "wayland_utils.h"
#include "window_wl_surface_role.h"
#include "wayland_input_dispatcher.h"

#include <mir/events/event_builders.h>

#include <mir/input/keymap.h>
#include <mir/log.h>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace geom = mir::geometry;
namespace mev = mir::events;
namespace mi = mir::input;

mf::WaylandSurfaceObserver::WaylandSurfaceObserver(
    WlSeat* seat,
    WlSurface* surface,
    WindowWlSurfaceRole* window)
    : seat{seat},
      window{window},
      input_dispatcher{std::make_unique<WaylandInputDispatcher>(seat, surface)},
      window_size{geometry::Size{0,0}},
      destroyed{std::make_shared<bool>(false)}
{
}

mf::WaylandSurfaceObserver::~WaylandSurfaceObserver()
{
    *destroyed = true;
}

void mf::WaylandSurfaceObserver::attrib_changed(ms::Surface const*, MirWindowAttrib attrib, int value)
{
    switch (attrib)
    {
    case mir_window_attrib_focus:
        run_on_wayland_thread_unless_destroyed([this, value]()
            {
                auto has_focus = static_cast<bool>(value);
                input_dispatcher->set_focus(has_focus);
                window->handle_active_change(has_focus);
            });
        break;

    case mir_window_attrib_state:
        run_on_wayland_thread_unless_destroyed([this, value]()
            {
                current_state = static_cast<MirWindowState>(value);
                window->handle_state_change(current_state);
            });
        break;

    default:;
    }
}

void mf::WaylandSurfaceObserver::content_resized_to(ms::Surface const*, geom::Size const& content_size)
{
    run_on_wayland_thread_unless_destroyed(
        [this, content_size]()
        {
            if (content_size != window_size)
            {
                requested_size = content_size;
                window->handle_resize(std::experimental::nullopt, content_size);
            }
        });
}

void mf::WaylandSurfaceObserver::client_surface_close_requested(ms::Surface const*)
{
    run_on_wayland_thread_unless_destroyed(
        [this]()
        {
            window->handle_close_request();
        });
}

void mf::WaylandSurfaceObserver::keymap_changed(
        ms::Surface const*,
        MirInputDeviceId /* id */,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options)
{
    // shared pointer instead of unique so it can be owned by the lambda
    auto const keymap = std::make_shared<mi::Keymap>(model, layout, variant, options);

    run_on_wayland_thread_unless_destroyed(
        [this, keymap]()
        {
            input_dispatcher->set_keymap(*keymap);
        });
}

void mf::WaylandSurfaceObserver::placed_relative(ms::Surface const*, geometry::Rectangle const& placement)
{
    run_on_wayland_thread_unless_destroyed(
        [this, placement = placement]()
        {
            requested_size = placement.size;
            window->handle_resize(placement.top_left, placement.size);
        });
}

void mf::WaylandSurfaceObserver::input_consumed(ms::Surface const*, MirEvent const* event)
{
    std::shared_ptr<MirEvent> owned_event = mev::clone_event(*event);

    run_on_wayland_thread_unless_destroyed(
        [this, owned_event]()
        {
            input_dispatcher->handle_event(owned_event.get());
        });
}

auto mf::WaylandSurfaceObserver::latest_timestamp() const -> std::chrono::nanoseconds
{
    return input_dispatcher->latest_timestamp();
}

void mf::WaylandSurfaceObserver::run_on_wayland_thread_unless_destroyed(std::function<void()>&& work)
{
    seat->spawn(run_unless(destroyed, work));
}
