/*
 * Copyright © 2013-2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/input/cursor_images.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/fake_event_hub_server_configuration.h"
#include "mir_test_framework/declarative_placement_strategy.h"
#include "mir_test_framework/using_stub_client_platform.h"

#include "mir_test/fake_shared.h"
#include "mir_test/spin_wait.h"
#include "mir_test/wait_condition.h"
#include "mir_test/fake_event_hub.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mt = mir::test;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;

namespace
{

struct MockCursor : public mg::Cursor
{
    MOCK_METHOD1(show, void(mg::CursorImage const&));
    MOCK_METHOD0(hide, void());

    // We are not interested in mocking the motion in these tests as we
    // generate it ourself.
    void move_to(geom::Point) override {}
};

struct NamedCursorImage : public mg::CursorImage
{
    NamedCursorImage(std::string const& name)
        : cursor_name(name)
    {
    }

    void const* as_argb_8888() const override { return nullptr; }
    geom::Size size() const override { return geom::Size{}; }
    geom::Displacement hotspot() const override { return geom::Displacement{0, 0}; }

    std::string const cursor_name;
};

struct NamedCursorImages : public mi::CursorImages
{
   std::shared_ptr<mg::CursorImage>
       image(std::string const& name, geom::Size const& /* size */) override
   {
       return std::make_shared<NamedCursorImage>(name);
   }
};

bool cursor_is_named(mg::CursorImage const& i, std::string const& name)
{
    auto image = dynamic_cast<NamedCursorImage const*>(&i);
    assert(image);

    return image->cursor_name == name;
}

MATCHER(DefaultCursorImage, "")
{
    return cursor_is_named(arg, "default");
}

MATCHER_P(CursorNamed, name, "")
{
    return cursor_is_named(arg, name);
}

struct CursorClient
{
    CursorClient(std::string const& connect_string, std::string const& client_name)
        : connect_string{connect_string}, client_name{client_name}
    {
    }

    virtual ~CursorClient()
    {
        teardown.wake_up_everyone();
        if (client_thread.joinable())
            client_thread.join();
    }

    void run()
    {
        mir::test::WaitCondition setup_done;

        client_thread = std::thread{
            [this,&setup_done]
            {
                auto const connection =
                    mir_connect_sync(connect_string.c_str(), client_name.c_str());

                auto spec = mir_connection_create_spec_for_normal_surface(connection,
                    1, 1, mir_pixel_format_abgr_8888);
                mir_surface_spec_set_name(spec, client_name.c_str());
                auto const surface = mir_surface_create_sync(spec);
                mir_surface_spec_release(spec);

                mir_surface_swap_buffers_sync(surface);

                wait_for_surface_to_become_focused_and_exposed(surface);

                setup_cursor(surface);

                setup_done.wake_up_everyone();

                teardown.wait_for_at_most_seconds(10);
                mir_surface_release_sync(surface);
                mir_connection_release(connection);
            }};

        setup_done.wait_for_at_most_seconds(5);
    }

    virtual void setup_cursor(MirSurface*)
    {
    }

    void wait_for_surface_to_become_focused_and_exposed(MirSurface* surface)
    {
        bool success = mt::spin_wait_for_condition_or_timeout(
            [surface]
            {
                return mir_surface_get_visibility(surface) == mir_surface_visibility_exposed &&
                       mir_surface_get_focus(surface) == mir_surface_focused;
            },
            std::chrono::seconds{5});

        if (!success)
            throw std::runtime_error("Timeout waiting for surface to become focused and exposed");
    }

    std::string const connect_string;
    std::string const client_name;

    std::thread client_thread;
    mir::test::WaitCondition teardown;
};

struct DisabledCursorClient : CursorClient
{
    using CursorClient::CursorClient;

    void setup_cursor(MirSurface* surface) override
    {
        auto conf = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
        mir_wait_for(mir_surface_configure_cursor(surface, conf));
        mir_cursor_configuration_destroy(conf);
    }
};

struct NamedCursorClient : CursorClient
{
    NamedCursorClient(
        std::string const& connect_string,
        std::string const& client_name,
        std::string const& cursor_name)
        : CursorClient{connect_string, client_name},
          cursor_name{cursor_name}
    {
    }

    void setup_cursor(MirSurface* surface) override
    {
        auto conf = mir_cursor_configuration_from_name(cursor_name.c_str());
        mir_wait_for(mir_surface_configure_cursor(surface, conf));
        mir_cursor_configuration_destroy(conf);
    }

    std::string const cursor_name;
};

struct TestServerConfiguration : mtf::FakeEventHubServerConfiguration
{
    std::shared_ptr<mir::scene::PlacementStrategy> the_placement_strategy() override
    {
        return std::make_shared<mtf::DeclarativePlacementStrategy>(
            FakeEventHubServerConfiguration::the_placement_strategy(),
            client_geometries, client_depths);
    }

    std::shared_ptr<mg::Cursor> the_cursor() override
    {
        return mt::fake_shared(cursor);
    }

    std::shared_ptr<mi::CursorImages> the_cursor_images() override
    {
        return std::make_shared<NamedCursorImages>();
    }

    MockCursor cursor;
    mtf::SurfaceGeometries client_geometries;
    mtf::SurfaceDepths client_depths;
};

struct TestClientCursorAPI : mtf::InProcessServer
{
    mir::DefaultServerConfiguration& server_config() override
    {
        return server_configuration_;
    }

    TestServerConfiguration& test_server_config()
    {
        return server_configuration_;
    }

    std::shared_ptr<mir::input::android::FakeEventHub> fake_event_hub()
    {
        return server_configuration_.fake_event_hub;
    }

    void client_shutdown_expectations()
    {
        using namespace testing;
        Mock::VerifyAndClearExpectations(&test_server_config().cursor);

        // Client shutdown
        EXPECT_CALL(test_server_config().cursor, show(_)).Times(AnyNumber());
        EXPECT_CALL(test_server_config().cursor, hide()).Times(AnyNumber());
    }

    std::string const client_name_1{"client-1"};
    std::string const client_name_2{"client-2"};
    std::string const client_cursor_1{"cursor-1"};
    std::string const client_cursor_2{"cursor-2"};
    TestServerConfiguration server_configuration_;
    mir::test::WaitCondition expectations_satisfied;
    mtf::UsingStubClientPlatform using_stub_client_platform;
};

}

// In this set we create a 1x1 client surface at the point (1,0). The client requests to disable the cursor
// over this surface. Since the cursor starts at (0,0) we when we move the cursor by (1,0) thus causing it
// to enter the bounds of the first surface, we should observe it being disabled.
TEST_F(TestClientCursorAPI, client_may_disable_cursor_over_surface)
{
    using namespace ::testing;

    test_server_config().client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};

    DisabledCursorClient client{new_connection(), client_name_1};
    client.run();

    EXPECT_CALL(test_server_config().cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 0));

    expectations_satisfied.wait_for_at_most_seconds(5);

    client_shutdown_expectations();
}

TEST_F(TestClientCursorAPI, cursor_restored_when_leaving_surface)
{
    using namespace ::testing;

    test_server_config().client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};

    DisabledCursorClient client{new_connection(), client_name_1};
    client.run();

    InSequence seq;
    EXPECT_CALL(test_server_config().cursor, hide());
    EXPECT_CALL(test_server_config().cursor, show(DefaultCursorImage()))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 0));
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(2, 0));

    expectations_satisfied.wait_for_at_most_seconds(5);

    client_shutdown_expectations();
}

TEST_F(TestClientCursorAPI, cursor_changed_when_crossing_surface_boundaries)
{
    using namespace ::testing;

    test_server_config().client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};
    test_server_config().client_geometries[client_name_2] =
        geom::Rectangle{{2, 0}, {1, 1}};

    NamedCursorClient client_1{new_connection(), client_name_1, client_cursor_1};
    NamedCursorClient client_2{new_connection(), client_name_2, client_cursor_2};
    client_1.run();
    client_2.run();

    InSequence seq;
    EXPECT_CALL(test_server_config().cursor, show(CursorNamed(client_cursor_1)));
    EXPECT_CALL(test_server_config().cursor, show(CursorNamed(client_cursor_2)))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 0));
    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 0));

    expectations_satisfied.wait_for_at_most_seconds(5);

    client_shutdown_expectations();
}

TEST_F(TestClientCursorAPI, cursor_request_taken_from_top_surface)
{
    using namespace ::testing;

    test_server_config().client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};
    test_server_config().client_geometries[client_name_2] =
        geom::Rectangle{{1, 0}, {1, 1}};

    NamedCursorClient client_1{new_connection(), client_name_1, client_cursor_1};
    NamedCursorClient client_2{new_connection(), client_name_2, client_cursor_2};
    client_1.run();
    client_2.run();

    EXPECT_CALL(test_server_config().cursor, show(CursorNamed(client_cursor_2)))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_event_hub()->synthesize_event(mis::a_motion_event().with_movement(1, 0));

    expectations_satisfied.wait_for_at_most_seconds(5);

    client_shutdown_expectations();
}

TEST_F(TestClientCursorAPI, cursor_request_applied_without_cursor_motion)
{
    using namespace ::testing;

    struct ChangingCursorClient : NamedCursorClient
    {
        using NamedCursorClient::NamedCursorClient;

        void setup_cursor(MirSurface* surface) override
        {
            auto conf1 = mir_cursor_configuration_from_name(cursor_name.c_str());
            auto conf2 = mir_cursor_configuration_from_name(mir_disabled_cursor_name);

            mir_wait_for(mir_surface_configure_cursor(surface, conf1));
            mir_wait_for(mir_surface_configure_cursor(surface, conf2));

            mir_cursor_configuration_destroy(conf1);
            mir_cursor_configuration_destroy(conf2);
        }
    };

    test_server_config().client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    ChangingCursorClient client{new_connection(), client_name_1, client_cursor_1};

    InSequence seq;
    EXPECT_CALL(test_server_config().cursor, show(CursorNamed(client_cursor_1)));
    EXPECT_CALL(test_server_config().cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    client.run();

    expectations_satisfied.wait_for_at_most_seconds(5);

    client_shutdown_expectations();
}
