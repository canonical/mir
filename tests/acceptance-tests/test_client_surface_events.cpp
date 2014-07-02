/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Nick Dedekind <nick.dedekind@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_toolkit/mir_client_library_debug.h"

#include "mir/shell/surface_coordinator_wrapper.h"

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

#include "mir_test_framework/stubbed_server_configuration.h"
#include "mir_test_framework/basic_client_server_fixture.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <condition_variable>
#include <chrono>
#include <mutex>

namespace mtf = mir_test_framework;
namespace ms = mir::scene;
namespace msh = mir::shell;

using namespace testing;

namespace
{
struct MockSurfaceCoordinator : msh::SurfaceCoordinatorWrapper
{
    MockSurfaceCoordinator(std::shared_ptr<ms::SurfaceCoordinator> const& wrapped) :
        msh::SurfaceCoordinatorWrapper(wrapped)
    {
    }

    std::shared_ptr<ms::Surface> add_surface(
        ms::SurfaceCreationParameters const& params,
        ms::Session* session) override
    {
        latest_surface = wrapped->add_surface(params, session);
        return latest_surface;
    }

    std::shared_ptr<ms::Surface> latest_surface;
};

struct MyConfig : mtf::StubbedServerConfiguration
{
    std::shared_ptr<ms::SurfaceCoordinator> wrap_surface_coordinator(
        std::shared_ptr<ms::SurfaceCoordinator> const& wrapped) override
    {
        auto const msc = std::make_shared<MockSurfaceCoordinator>(wrapped);
        mock_surface_coordinator = msc;
        return msc;
    }

    std::shared_ptr<MockSurfaceCoordinator> the_mock_surface_coordinator() const
    {
        return mock_surface_coordinator.lock();
    }

    std::shared_ptr<ms::Surface> the_latest_surface() const
    {
        return the_mock_surface_coordinator()->latest_surface;
    }

    std::weak_ptr<MockSurfaceCoordinator> mock_surface_coordinator;
};

using BasicClientServerFixture = mtf::BasicClientServerFixture<MyConfig>;

struct ClientSurfaceEvents : BasicClientServerFixture
{
    MirSurfaceParameters const request_params
    {
        __FILE__,
        640, 480,
        mir_pixel_format_abgr_8888,
        mir_buffer_usage_hardware,
        mir_display_output_id_invalid
    };

    MirSurface* surface{nullptr};
    MirSurface* other_surface;

    std::mutex last_event_mutex;
    MirEventType event_filter{mir_event_type_surface};
    std::condition_variable last_event_cv;
    MirEvent last_event{};
    MirSurface* last_event_surface = nullptr;
    MirEventDelegate delegate{&event_callback, this};

    std::shared_ptr<ms::Surface> scene_surface;

    static void event_callback(MirSurface* surface, MirEvent const* event, void* ctx)
    {
        ClientSurfaceEvents* self = static_cast<ClientSurfaceEvents*>(ctx);
        std::lock_guard<decltype(self->last_event_mutex)> last_event_lock{self->last_event_mutex};
        // Don't overwrite an interesting event with an uninteresting one!
        if (event->type != self->event_filter) return;
        self->last_event = *event;
        self->last_event_surface = surface;
        self->last_event_cv.notify_one();
    }

    bool wait_for_event(std::chrono::milliseconds delay)
    {
        std::unique_lock<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        return last_event_cv.wait_for(last_event_lock, delay,
            [&] { return last_event_surface == surface && last_event.type == event_filter; });
    }

    void set_event_filter(MirEventType type)
    {
        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        event_filter = type;
    }

    void reset_last_event()
    {
        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        memset(&last_event, 0, sizeof last_event);
        last_event_surface = nullptr;
    }

    void SetUp() override
    {
        BasicClientServerFixture::SetUp();

        surface = mir_connection_create_surface_sync(connection, &request_params);
        mir_surface_set_event_handler(surface, &delegate);

        scene_surface = server_configuration.the_latest_surface();

        other_surface = mir_connection_create_surface_sync(connection, &request_params);
        mir_surface_set_event_handler(other_surface, nullptr);

        reset_last_event();
    }

    void TearDown() override
    {
        mir_surface_release_sync(other_surface);
        scene_surface.reset();
        mir_surface_release_sync(surface);

        BasicClientServerFixture::TearDown();
    }
};
}

TEST_F(ClientSurfaceEvents, surface_receives_state_events)
{
    int surface_id = mir_debug_surface_id(surface);

    {
        mir_wait_for(mir_surface_set_state(surface, mir_surface_state_fullscreen));
        mir_wait_for(mir_surface_set_state(other_surface, mir_surface_state_minimized));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_THAT(last_event_surface, Eq(surface));
        EXPECT_THAT(last_event.type, Eq(mir_event_type_surface));
        EXPECT_THAT(last_event.surface.id, Eq(surface_id));
        EXPECT_THAT(last_event.surface.attrib, Eq(mir_surface_attrib_state));
        EXPECT_THAT(last_event.surface.value, Eq(mir_surface_state_fullscreen));
    }

    {
        mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(999)));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_THAT(last_event_surface, Eq(surface));
        EXPECT_THAT(last_event.type, Eq(mir_event_type_surface));
        EXPECT_THAT(last_event.surface.id, Eq(surface_id));
        EXPECT_THAT(last_event.surface.attrib, Eq(mir_surface_attrib_state));
        EXPECT_THAT(last_event.surface.value, Eq(mir_surface_state_fullscreen));
    }

    reset_last_event();

    {
        mir_wait_for(mir_surface_set_state(surface, mir_surface_state_minimized));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_THAT(last_event_surface, Eq(surface));
        EXPECT_THAT(last_event.type, Eq(mir_event_type_surface));
        EXPECT_THAT(last_event.surface.id, Eq(surface_id));
        EXPECT_THAT(last_event.surface.attrib, Eq(mir_surface_attrib_state));
        EXPECT_THAT(last_event.surface.value, Eq(mir_surface_state_minimized));
    }

    reset_last_event();

    {
        mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(777)));
        mir_wait_for(mir_surface_set_state(other_surface, mir_surface_state_maximized));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_THAT(last_event_surface, IsNull());
        EXPECT_THAT(last_event.type, Eq(0));
        EXPECT_THAT(last_event.surface.id, Eq(0));
        EXPECT_THAT(last_event.surface.attrib, Eq(0));
        EXPECT_THAT(last_event.surface.value, Eq(0));
    }
}

struct OrientationEvents : ClientSurfaceEvents, ::testing::WithParamInterface<MirOrientation> {};

TEST_P(OrientationEvents, surface_receives_orientation_events)
{
    set_event_filter(mir_event_type_orientation);

    auto const direction = GetParam();

    scene_surface->set_orientation(direction);

    EXPECT_TRUE(wait_for_event(std::chrono::seconds(1)));

    std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

    EXPECT_THAT(last_event_surface, Eq(surface));
    EXPECT_THAT(last_event.type, Eq(mir_event_type_orientation));
    EXPECT_THAT(last_event.orientation.direction, Eq(direction));
}

INSTANTIATE_TEST_CASE_P(ClientSurfaceEvents,
    OrientationEvents,
    Values(mir_orientation_normal, mir_orientation_left, mir_orientation_inverted, mir_orientation_right));

TEST_F(ClientSurfaceEvents, client_can_query_current_orientation)
{
    set_event_filter(mir_event_type_orientation);

    for (auto const direction:
        {mir_orientation_normal, mir_orientation_left, mir_orientation_inverted,
         mir_orientation_right, mir_orientation_normal, mir_orientation_inverted,
         mir_orientation_left, mir_orientation_inverted, mir_orientation_right})
    {
        reset_last_event();

        scene_surface->set_orientation(direction);

        EXPECT_TRUE(wait_for_event(std::chrono::seconds(1)));

        EXPECT_THAT(mir_surface_get_orientation(surface), Eq(direction));
    }
}
