/*
 * Copyright © 2020 Canonical Ltd.
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
 * Authored by: William Wold <william.wold@canonical.com>
 */

#include "xwayland_surface_observer.h"
#include "xwayland_wm_surface.h"
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

mf::XWaylandSurfaceObserver::XWaylandSurfaceObserver(
    WlSeat& seat,
    WlSurface* wl_surface,
    XWaylandWMSurface* wm_surface)
    : wm_surface{wm_surface},
      input_dispatcher{std::make_shared<ThreadsafeInputDispatcher>(
          std::make_unique<WaylandInputDispatcher>(&seat, wl_surface))}
{
}

mf::XWaylandSurfaceObserver::~XWaylandSurfaceObserver()
{
    std::lock_guard<std::mutex> lock{input_dispatcher->mutex};
    input_dispatcher->dispatcher = std::experimental::nullopt;
}

void mf::XWaylandSurfaceObserver::attrib_changed(ms::Surface const*, MirWindowAttrib attrib, int value)
{
    switch (attrib)
    {
    case mir_window_attrib_focus:
    {
        auto has_focus = static_cast<bool>(value);
        /// TODO: update the active state on the wm_window
        aquire_input_dispatcher(
            [has_focus](auto input_dispatcher)
            {
                input_dispatcher->set_focus(has_focus);
            });
    }   break;

    case mir_window_attrib_state:
    {
        auto state = static_cast<MirWindowState>(value);
        wm_surface->apply_mir_state_to_window(state);
    }   break;

    default:;
    }
}

void mf::XWaylandSurfaceObserver::content_resized_to(ms::Surface const*, geom::Size const& content_size)
{
    wm_surface->send_resize(content_size);
}

void mf::XWaylandSurfaceObserver::client_surface_close_requested(ms::Surface const*)
{
    wm_surface->send_close_request();
}

void mf::XWaylandSurfaceObserver::keymap_changed(
        ms::Surface const*,
        MirInputDeviceId /* id */,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options)
{
    // shared pointer instead of unique so it can be owned by the lambda
    auto const keymap = std::make_shared<mi::Keymap>(model, layout, variant, options);

    aquire_input_dispatcher(
        [keymap](auto input_dispatcher)
        {
            input_dispatcher->set_keymap(*keymap);
        });
}

void mf::XWaylandSurfaceObserver::input_consumed(ms::Surface const*, MirEvent const* event)
{
    std::shared_ptr<MirEvent> owned_event = mev::clone_event(*event);

    aquire_input_dispatcher(
        [owned_event](auto input_dispatcher)
        {
            input_dispatcher->handle_event(owned_event.get());
        });
}

auto mf::XWaylandSurfaceObserver::latest_timestamp() const -> std::chrono::nanoseconds
{
    std::lock_guard<std::mutex> lock{input_dispatcher->mutex};
    if (input_dispatcher->dispatcher)
        return input_dispatcher->dispatcher.value()->latest_timestamp();
    else
        return {};
}

mf::XWaylandSurfaceObserver::ThreadsafeInputDispatcher::ThreadsafeInputDispatcher(
    std::unique_ptr<WaylandInputDispatcher> dispatcher)
    : dispatcher{move(dispatcher)}
{
}

mf::XWaylandSurfaceObserver::ThreadsafeInputDispatcher::~ThreadsafeInputDispatcher()
{
}

void mf::XWaylandSurfaceObserver::aquire_input_dispatcher(std::function<void(WaylandInputDispatcher*)>&& work)
{
    wm_surface->run_on_wayland_thread([work = move(work), input_dispatcher = input_dispatcher]()
        {
            std::lock_guard<std::mutex> lock{input_dispatcher->mutex};
            if (input_dispatcher->dispatcher)
            {
                work(input_dispatcher->dispatcher.value().get());
            }
        });
}
