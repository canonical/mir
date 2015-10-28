/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/test/wait_condition.h"
#include "mir/test/event_matchers.h"

#include "mir_test_framework/connected_client_headless_server.h"
#include "mir_test_framework/process.h"
#include "mir/test/cross_process_sync.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace ::testing;

namespace
{
struct FocusLogger
{
    void log_focus_event(MirSurface* surface, MirSurfaceFocusState state)
    {
        std::lock_guard<std::mutex> lk(mutex);
        focus_events[surface].push_back(state);
        cv.notify_all();
    }

    void wait_for_num_focus_events(unsigned int num, MirSurface* surface, std::chrono::seconds until)
    {
        std::unique_lock<std::mutex> lk(mutex);
        if (!cv.wait_for(lk, until, [this, num, surface]
            {
                return ((focus_events.find(surface) != focus_events.end()) &&
                    (focus_events[surface].size() >= num));
            }))
        {
            throw std::logic_error("timeout waiting for events");
        }
    }

    std::vector<MirSurfaceFocusState> events_for(MirSurface* surface)
    {
        std::lock_guard<std::mutex> lk(mutex);
        if (focus_events.find(surface) == focus_events.end()) 
            throw std::logic_error("no events");
        return focus_events[surface];
    }

    static void handle_event(MirSurface* surface, MirEvent const* ev, void* context)
    {
        if (mir_event_type_surface == mir_event_get_type(ev))
        {
            auto surface_ev = mir_event_get_surface_event(ev);
            if (mir_surface_attrib_focus == mir_surface_event_get_attribute(surface_ev))
            {
                auto self = static_cast<FocusLogger*>(context);
                self->log_focus_event(surface,
                    static_cast<MirSurfaceFocusState>(mir_surface_event_get_attribute_value(surface_ev)));
            }
        }
    }
private:
    std::mutex mutex;
    std::condition_variable cv;
    std::map<MirSurface*, std::vector<MirSurfaceFocusState>> focus_events;
};

struct FocusSurface
{
    FocusSurface(MirConnection* connection, std::shared_ptr<FocusLogger> const& logger)
    {
        auto spec = mir_connection_create_spec_for_normal_surface(connection, 100, 100, mir_pixel_format_abgr_8888);
        mir_surface_spec_set_event_handler(spec, FocusLogger::handle_event, logger.get());

        surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    ~FocusSurface()
    {
        if (!released) release();
    }

    MirSurface* native_handle() const
    {
        return surface;
    }

    void release()
    {
        mir_surface_release_sync(surface);
        released = true;
    }

    MirSurface* surface;
    bool released{false};
};

struct ClientFocusNotification : mtf::ConnectedClientHeadlessServer
{
    std::shared_ptr<FocusLogger> logger{std::make_shared<FocusLogger>()};
};
}

TEST_F(ClientFocusNotification, a_surface_is_notified_of_receiving_focus)
{
    FocusSurface surface(connection, logger);
    surface.release();

    logger->wait_for_num_focus_events(2, surface.native_handle(), std::chrono::seconds(5));

    auto log = logger->events_for(surface.native_handle());
    EXPECT_THAT(log[0], Eq(mir_surface_focused));
    EXPECT_THAT(log[1], Eq(mir_surface_unfocused));
}

TEST_F(ClientFocusNotification, two_surfaces_are_notified_of_gaining_and_losing_focus)
{
    FocusSurface surface1(connection, logger);
    logger->wait_for_num_focus_events(1, surface1.native_handle(), std::chrono::seconds(5));
    FocusSurface surface2(connection, logger);
    surface2.release();
    surface1.release();

    logger->wait_for_num_focus_events(4, surface1.native_handle(), std::chrono::seconds(5));
    logger->wait_for_num_focus_events(2, surface2.native_handle(), std::chrono::seconds(5));

    auto log = logger->events_for(surface1.native_handle());
    EXPECT_THAT(log[0], Eq(mir_surface_focused));
    EXPECT_THAT(log[1], Eq(mir_surface_unfocused));
    EXPECT_THAT(log[2], Eq(mir_surface_focused));
    EXPECT_THAT(log[3], Eq(mir_surface_unfocused));

    log = logger->events_for(surface2.native_handle());
    EXPECT_THAT(log[0], Eq(mir_surface_focused));
    EXPECT_THAT(log[1], Eq(mir_surface_unfocused));
}
