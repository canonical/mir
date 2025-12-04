/*
 * Copyright © Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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

#ifndef MIR_TEST_DOUBLES_TEST_SURFACE_OBSERVER_H_
#define MIR_TEST_DOUBLES_TEST_SURFACE_OBSERVER_H_

#include <mir/scene/null_surface_observer.h>
#include <mir/frontend/surface_id.h>
#include <mir/frontend/event_sink.h>
#include <mir/events/event_builders.h>
#include <mir/geometry/size.h>

#include <memory>

namespace mir
{
namespace test
{
namespace doubles
{

/// Test observer that forwards surface events to an event sink
class TestSurfaceObserver : public scene::NullSurfaceObserver
{
public:
    TestSurfaceObserver(
        frontend::SurfaceId surface_id,
        std::shared_ptr<frontend::EventSink> const& event_sink)
        : surface_id(surface_id),
          event_sink(event_sink)
    {
    }

    void content_resized_to(scene::Surface const*, geometry::Size const& content_size) override
    {
        event_sink->handle_event(events::make_window_resize_event(surface_id, content_size));
    }

    void input_consumed(scene::Surface const*, std::shared_ptr<MirEvent const> const& event) override
    {
        auto ev = events::clone_event(*event);
        events::set_window_id(*ev, surface_id.as_value());
        event_sink->handle_event(std::move(ev));
    }

    void attrib_changed(scene::Surface const*, MirWindowAttrib attrib, int value) override
    {
        event_sink->handle_event(events::make_window_configure_event(surface_id, attrib, value));
    }

    void client_surface_close_requested(scene::Surface const*) override
    {
        event_sink->handle_event(events::make_window_close_event(surface_id));
    }

private:
    frontend::SurfaceId const surface_id;
    std::shared_ptr<frontend::EventSink> const event_sink;
};

}
}
}

#endif // MIR_TEST_DOUBLES_TEST_SURFACE_OBSERVER_H_
