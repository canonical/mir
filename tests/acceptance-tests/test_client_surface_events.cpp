/*
 * Copyright © 2014-2015 Canonical Ltd.
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

#include "mir/scene/surface.h"
#include "mir/scene/surface_creation_parameters.h"

#include "mir/graphics/display.h"

#include "mir/shell/display_configuration_controller.h"

#include "mir/test/event_matchers.h"
#include "mir/test/doubles/wrap_shell_to_track_latest_surface.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir_test_framework/any_surface.h"
#include "mir/test/signal.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <condition_variable>
#include <chrono>
#include <mutex>

namespace mf = mir::frontend;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;
namespace mg = mir::graphics;

using namespace testing;

namespace
{
struct ClientSurfaceEvents : mtf::ConnectedClientWithASurface
{
    MirSurface* other_surface;

    std::mutex last_event_mutex;
    MirEventType event_filter{mir_event_type_surface};
    std::condition_variable last_event_cv;
    MirEvent const* last_event = nullptr;
    MirSurface* last_event_surface = nullptr;

    std::shared_ptr<ms::Surface> scene_surface;

    ~ClientSurfaceEvents()
    {
        if (last_event)
            mir_event_unref(last_event);
    }

    static void event_callback(MirSurface* surface, MirEvent const* event, void* ctx)
    {
        ClientSurfaceEvents* self = static_cast<ClientSurfaceEvents*>(ctx);
        std::lock_guard<decltype(self->last_event_mutex)> last_event_lock{self->last_event_mutex};
        // Don't overwrite an interesting event with an uninteresting one!
        if (mir_event_get_type(event) != self->event_filter) return;
        
        if (self->last_event)
            mir_event_unref(self->last_event);
        
        self->last_event = mir_event_ref(event);
        self->last_event_surface = surface;
        self->last_event_cv.notify_one();
    }

    bool wait_for_event(std::chrono::milliseconds delay)
    {
        std::unique_lock<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        return last_event_cv.wait_for(last_event_lock, delay,
            [&] { return last_event_surface == surface && mir_event_get_type(last_event) == event_filter; });
    }

    void set_event_filter(MirEventType type)
    {
        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        event_filter = type;
    }

    void reset_last_event()
    {
        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        if (last_event != nullptr)
            mir_event_unref(last_event);
        last_event = nullptr;
        last_event_surface = nullptr;
    }

    std::shared_ptr<mtd::WrapShellToTrackLatestSurface> the_mock_shell() const
    {
        return mock_shell.lock();
    }

    std::shared_ptr<ms::Surface> the_latest_surface() const
    {
        return the_mock_shell()->latest_surface.lock();
    }

    void SetUp() override
    {
        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
            -> std::shared_ptr<msh::Shell>
        {
            auto const msc = std::make_shared<mtd::WrapShellToTrackLatestSurface>(wrapped);
            mock_shell = msc;
            return msc;
        });

        mtf::ConnectedClientWithASurface::SetUp();

        mir_surface_set_event_handler(surface, &event_callback, this);

        scene_surface = the_latest_surface();

        other_surface = mtf::make_any_surface(connection);
        mir_surface_set_event_handler(other_surface, nullptr, nullptr);

        reset_last_event();
    }

    void TearDown() override
    {
        mir_surface_release_sync(other_surface);
        scene_surface.reset();

        mtf::ConnectedClientWithASurface::TearDown();
    }

    std::weak_ptr<mtd::WrapShellToTrackLatestSurface> mock_shell;
};
}

TEST_F(ClientSurfaceEvents, surface_receives_state_events)
{
    {
        mir_wait_for(mir_surface_set_state(surface, mir_surface_state_fullscreen));
        mir_wait_for(mir_surface_set_state(other_surface, mir_surface_state_vertmaximized));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_THAT(last_event, mt::SurfaceEvent(mir_surface_attrib_state, mir_surface_state_fullscreen));
    }

    {
        mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(999)));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};
        EXPECT_THAT(last_event, mt::SurfaceEvent(mir_surface_attrib_state, mir_surface_state_fullscreen));
    }

    reset_last_event();

    {
        mir_wait_for(mir_surface_set_state(surface, mir_surface_state_vertmaximized));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_THAT(last_event, mt::SurfaceEvent(mir_surface_attrib_state, mir_surface_state_vertmaximized));
    }

    reset_last_event();

    {
        mir_wait_for(mir_surface_set_state(surface, static_cast<MirSurfaceState>(777)));
        mir_wait_for(mir_surface_set_state(other_surface, mir_surface_state_maximized));

        std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

        EXPECT_EQ(nullptr, last_event);
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

    EXPECT_THAT(last_event, mt::OrientationEvent(direction));
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

TEST_F(ClientSurfaceEvents, surface_receives_close_event)
{
    set_event_filter(mir_event_type_close_surface);

    scene_surface->request_client_surface_close();

    EXPECT_TRUE(wait_for_event(std::chrono::seconds(1)));

    std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

    EXPECT_THAT(last_event_surface, Eq(surface));
    EXPECT_THAT(mir_event_get_type(last_event), Eq(mir_event_type_close_surface));
}

TEST_F(ClientSurfaceEvents, client_can_query_preferred_orientation)
{

    for (auto const mode:
        {mir_orientation_mode_portrait, mir_orientation_mode_portrait_inverted,
         mir_orientation_mode_landscape, mir_orientation_mode_landscape_inverted,
         mir_orientation_mode_portrait_any, mir_orientation_mode_landscape_any,
         mir_orientation_mode_any})
    {
        reset_last_event();

        mir_wait_for(mir_surface_set_preferred_orientation(surface, mode));
        EXPECT_THAT(mir_surface_get_preferred_orientation(surface), Eq(mode));
    }
}

TEST_F(ClientSurfaceEvents, surface_receives_output_event_when_configuration_changes)
{
    using namespace std::literals::chrono_literals;

    auto constexpr form_factor = mir_form_factor_tablet;
    float constexpr scale = 2.15f;

    auto display_configuration = server.the_display()->configuration();

    display_configuration->for_each_output(
        [](mg::UserDisplayConfigurationOutput& output_config)
        {
            output_config.scale = scale;
            output_config.form_factor = form_factor;
        });

    set_event_filter(mir_event_type_surface_output);
    reset_last_event();

    auto display_controller = server.the_display_configuration_controller();
    display_controller->set_default_display_configuration(std::move(display_configuration)).get();

    ASSERT_TRUE(wait_for_event(1min));

    std::lock_guard<decltype(last_event_mutex)> last_event_lock{last_event_mutex};

    EXPECT_THAT(mir_event_get_type(last_event), Eq(mir_event_type_surface_output));
    auto output_event = mir_event_get_surface_output_event(last_event);

    EXPECT_THAT(mir_surface_output_event_get_form_factor(output_event), Eq(form_factor));
    EXPECT_THAT(mir_surface_output_event_get_scale(output_event), FloatEq(scale));
}

namespace
{
class WrapShellGeneratingCloseEvent : public mir::shell::ShellWrapper
{
    using mir::shell::ShellWrapper::ShellWrapper;

    mir::frontend::SurfaceId create_surface(
        std::shared_ptr <mir::scene::Session> const& session,
        mir::scene::SurfaceCreationParameters const& params,
        std::shared_ptr<mir::frontend::EventSink> const& sink) override
    {
        auto const surface = mir::shell::ShellWrapper::create_surface(session, params, sink);
        session->surface(surface)->request_client_surface_close();
        return surface;
    }
};

class ClientSurfaceStartupEvents : public mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
            -> std::shared_ptr<msh::Shell>
            {
                return std::make_shared<WrapShellGeneratingCloseEvent>(wrapped);
            });

        mtf::ConnectedClientHeadlessServer::SetUp();
    }
};

void raise_signal_on_close_event(MirSurface*, MirEvent const* ev, void* ctx)
{
    if (mir_event_get_type(ev) == mir_event_type_close_surface)
    {
        auto signal = reinterpret_cast<mt::Signal*>(ctx);
        signal->raise();
    }
}
}

TEST_F(ClientSurfaceStartupEvents, receives_event_sent_during_surface_construction)
{
    mt::Signal done;

    auto spec = mir_connection_create_spec_for_normal_surface(connection, 100, 100, mir_pixel_format_abgr_8888);
    mir_surface_spec_set_event_handler(spec, &raise_signal_on_close_event, &done);

    auto surface = mir_surface_create_sync(spec);

    mir_surface_spec_release(spec);

    /* This expectation will fail if the event generated during surface creation is
     * sent before the create_surface reply.
     *
     * In that case, libmirclient first receives a close_surface event for a surface
     * it doesn't know about, throws it away, and then receives the SurfaceID of the
     * surface it just created.
     */
    EXPECT_TRUE(done.wait_for(std::chrono::seconds{10}));

    mir_surface_release_sync(surface);
}

struct EventContext
{
    EventContext()
        : event{nullptr}
    {
    }

    ~EventContext()
    {
        if (event != nullptr)
            mir_event_unref(event);
    }

    mt::Signal captured;
    MirEvent const* event;
};

void surface_output_capturing_callback(MirSurface*, MirEvent const* ev, void* ctx)
{
    if (mir_event_get_type(ev) == mir_event_type_surface_output)
    {
        auto out_event = reinterpret_cast<EventContext*>(ctx);
        out_event->event = mir_event_ref(ev);
        out_event->captured.raise();
    }
}

TEST_F(ClientSurfaceEvents, surface_receives_output_event_on_creation)
{
    using namespace std::literals::chrono_literals;

    auto constexpr form_factor = mir_form_factor_tablet;
    float constexpr scale = 2.15f;

    std::shared_ptr<mg::DisplayConfiguration> display_configuration{server.the_display()->configuration()};

    display_configuration->for_each_output(
        [](mg::UserDisplayConfigurationOutput& output_config)
        {
            output_config.scale = scale;
            output_config.form_factor = form_factor;
        });

    auto display_controller = server.the_display_configuration_controller();
    display_controller->set_default_display_configuration(std::move(display_configuration)).get();

    EventContext context;

    auto spec = mir_connection_create_spec_for_normal_surface(connection, 640, 480, mir_pixel_format_abgr_8888);
    mir_surface_spec_set_event_handler(spec, &surface_output_capturing_callback, &context);
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);

    ASSERT_TRUE(context.captured.wait_for(10s));
    ASSERT_THAT(mir_event_get_type(context.event), Eq(mir_event_type_surface_output));
    auto surface_event = mir_event_get_surface_output_event(context.event);
    EXPECT_THAT(mir_surface_output_event_get_form_factor(surface_event), Eq(form_factor));
    EXPECT_THAT(mir_surface_output_event_get_scale(surface_event), Eq(scale));

    mir_surface_release_sync(surface);
}
