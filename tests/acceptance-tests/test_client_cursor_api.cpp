/*
 * Copyright © 2013-2016 Canonical Ltd.
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
#include "mir/input/input_device_info.h"
#include "mir/scene/surface_observer.h"

#include "mir_test_framework/in_process_server.h"
#include "mir_test_framework/executable_path.h"
#include "mir_test_framework/fake_input_device.h"
#include "mir_test_framework/stub_server_platform_factory.h"
#include "mir_test_framework/headless_in_process_server.h"
#include "mir_test_framework/declarative_placement_window_manage_policy.h"
#include "mir_test_framework/headless_nested_server_runner.h"
#include "mir_test_framework/observant_shell.h"
#include "mir/test/doubles/mock_egl.h"

#include "mir/test/fake_shared.h"
#include "mir/test/spin_wait.h"
#include "mir/test/signal.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;
namespace ms = mir::scene;
namespace msh = mir::shell;
namespace msc = mir::scene;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

namespace
{

std::chrono::seconds const timeout{5};
class MockSurfaceObserver : public msc::SurfaceObserver
{
public:
    MOCK_METHOD2(attrib_changed, void(MirWindowAttrib attrib, int value));
    MOCK_METHOD1(resized_to, void(geom::Size const& size));
    MOCK_METHOD1(moved_to, void(geom::Point const& top_left));
    MOCK_METHOD1(hidden_set_to, void(bool hide));
    MOCK_METHOD2(frame_posted, void(int frames_available, geom::Size const& size));
    MOCK_METHOD1(alpha_set_to, void(float alpha));
    MOCK_METHOD1(orientation_set_to, void(MirOrientation orientation));
    MOCK_METHOD1(transformation_set_to, void(glm::mat4 const& t));
    MOCK_METHOD1(reception_mode_set_to, void(mi::InputReceptionMode mode));
    MOCK_METHOD1(cursor_image_set_to, void(mg::CursorImage const& image));
    MOCK_METHOD0(client_surface_close_requested, void());
    MOCK_METHOD5(keymap_changed, void(
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options));
    MOCK_METHOD1(renamed, void(char const* name));
    MOCK_METHOD0(cursor_image_removed, void());
    MOCK_METHOD1(placed_relative, void(geom::Rectangle const& placement));
};


struct MockCursor : public mg::Cursor
{
    MOCK_METHOD0(show, void());
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
    geom::Size size() const override { return geom::Size{16, 16}; }
    geom::Displacement hotspot() const override { return geom::Displacement{0, 0}; }

    std::string const cursor_name;
};

struct NamedCursorImages : public mi::CursorImages
{
    std::shared_ptr<mg::CursorImage>
       image(std::string const& name, geom::Size const& /* size */) override
    {
        if (name == "none")
            return nullptr;
    
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
        teardown.raise();
        if (client_thread.joinable())
            client_thread.join();
    }

    void run()
    {
        mir::test::Signal setup_done;

        client_thread = std::thread{
            [this,&setup_done]
            {
                connection =
                    mir_connect_sync(connect_string.c_str(), client_name.c_str());

                auto spec = mir_create_normal_window_spec(connection, 1, 1);
                mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
                mir_window_spec_set_name(spec, client_name.c_str());
                auto const window = mir_create_window_sync(spec);
                mir_window_spec_release(spec);

                mir_buffer_stream_swap_buffers_sync(
                    mir_window_get_buffer_stream(window));

                wait_for_surface_to_become_focused_and_exposed(window);

                setup_cursor(window);

                setup_done.raise();

                teardown.wait_for(std::chrono::seconds{10});
                mir_window_release_sync(window);
                mir_connection_release(connection);
            }};

        setup_done.wait_for(std::chrono::seconds{5});
    }

    virtual void setup_cursor(MirWindow*)
    {
    }

    void wait_for_surface_to_become_focused_and_exposed(MirWindow* window)
    {
        bool success = mt::spin_wait_for_condition_or_timeout(
            [window]
            {
                return mir_window_get_visibility(window) == mir_window_visibility_exposed &&
                    mir_window_get_focus_state(window) == mir_window_focus_state_focused;
            },
            std::chrono::seconds{5});

        if (!success)
            throw std::runtime_error("Timeout waiting for window to become focused and exposed");
    }

    std::string const connect_string;
    std::string const client_name;

    MirConnection* connection;

    std::thread client_thread;
    mir::test::Signal teardown;
};

struct DisabledCursorClient : CursorClient
{
    using CursorClient::CursorClient;

    void setup_cursor(MirWindow* window) override
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto conf = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
#pragma GCC diagnostic pop
        mir_window_configure_cursor(window, conf);
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

    void setup_cursor(MirWindow* window) override
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto conf = mir_cursor_configuration_from_name(cursor_name.c_str());
#pragma GCC diagnostic pop
        mir_window_configure_cursor(window, conf);
        mir_cursor_configuration_destroy(conf);
    }

    std::string const cursor_name;
};

struct TestClientCursorAPI : mtf::HeadlessInProcessServer
{
    // mtf::add_fake_input_device needs this library to be loaded each test, for the tests
    mtf::TemporaryEnvironmentValue input_lib{"MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str()};
    ::testing::NiceMock<MockCursor> cursor;
    std::shared_ptr<::testing::NiceMock<MockSurfaceObserver>> mock_surface_observer =
        std::make_shared<::testing::NiceMock<MockSurfaceObserver>>();

    mtf::SurfaceGeometries client_geometries;

    TestClientCursorAPI()
    {
        mock_egl.provide_egl_extensions();
        mock_egl.provide_stub_platform_buffer_swapping();

        server.wrap_cursor([this](std::shared_ptr<mg::Cursor> const&) { return mt::fake_shared(cursor); });

        server.override_the_cursor_images([]() { return std::make_shared<NamedCursorImages>(); });

        server.override_the_window_manager_builder([this](msh::FocusController* focus_controller)
            {
                using PlacementWindowManager = msh::WindowManagerConstructor<mtf::DeclarativePlacementWindowManagerPolicy>;
                return std::make_shared<PlacementWindowManager>(
                    focus_controller,
                    client_geometries,
                    server.the_shell_display_layout());
            });

        server.wrap_shell([&, this](auto const& wrapped)
        {
            return std::make_shared<mtf::ObservantShell>(wrapped, mock_surface_observer);
        });
    }

    void expect_client_shutdown()
    {
        using namespace testing;
        Mock::VerifyAndClearExpectations(&cursor);

        // Client shutdown
        EXPECT_CALL(cursor, show(_)).Times(AnyNumber());
        EXPECT_CALL(cursor, hide()).Times(AnyNumber());
    }

    std::string const client_name_1{"client-1"};
    std::string const client_name_2{"client-2"};
    std::string const client_cursor_1{"cursor-1"};
    std::string const client_cursor_2{"cursor-2"};

    mir::test::Signal expectations_satisfied;

    std::unique_ptr<mtf::FakeInputDevice> fake_mouse{
        mtf::add_fake_input_device(mi::InputDeviceInfo{"mouse", "mouse-uid" , mi::DeviceCapability::pointer})
        };

    ::testing::NiceMock<mtd::MockEGL> mock_egl;
};

}

// In this set we create a 1x1 client window at the point (1,0). The client requests to disable the cursor
// over this window. Since the cursor starts at (0,0) we when we move the cursor by (1,0) thus causing it
// to enter the bounds of the first window, we should observe it being disabled.
TEST_F(TestClientCursorAPI, client_may_disable_cursor_over_surface)
{
    using namespace ::testing;

    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_removed())
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    DisabledCursorClient client{new_connection(), client_name_1};
    client.run();

    EXPECT_TRUE(wait.wait_for(timeout));

    EXPECT_CALL(cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));

    expectations_satisfied.wait_for(std::chrono::seconds{5});

    expect_client_shutdown();
}

TEST_F(TestClientCursorAPI, cursor_restored_when_leaving_surface)
{
    using namespace ::testing;

    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_removed())
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    DisabledCursorClient client{new_connection(), client_name_1};
    client.run();

    EXPECT_TRUE(wait.wait_for(timeout));

    InSequence seq;
    EXPECT_CALL(cursor, hide());
    EXPECT_CALL(cursor, show(DefaultCursorImage()))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(2, 0));

    expectations_satisfied.wait_for(std::chrono::seconds{5});

    expect_client_shutdown();
}

TEST_F(TestClientCursorAPI, cursor_changed_when_crossing_surface_boundaries)
{
    using namespace ::testing;

    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};
    client_geometries[client_name_2] =
        geom::Rectangle{{2, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_1{new_connection(), client_name_1, client_cursor_1};
    client_1.run();

    EXPECT_TRUE(wait.wait_for(timeout));
    wait.reset();

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_2{new_connection(), client_name_2, client_cursor_2};
    client_2.run();

    EXPECT_TRUE(wait.wait_for(timeout));

    InSequence seq;
    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_1)));
    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_2)))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));

    expectations_satisfied.wait_for(std::chrono::seconds{5});

    expect_client_shutdown();
}

TEST_F(TestClientCursorAPI, cursor_request_taken_from_top_surface)
{
    using namespace ::testing;

    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};
    client_geometries[client_name_2] =
        geom::Rectangle{{1, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_1{new_connection(), client_name_1, client_cursor_1};
    client_1.run();

    EXPECT_TRUE(wait.wait_for(timeout));
    wait.reset();

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_2{new_connection(), client_name_2, client_cursor_2};
    client_2.run();

    EXPECT_TRUE(wait.wait_for(timeout));

    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_2)))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));

    expectations_satisfied.wait_for(std::chrono::seconds{5});

    expect_client_shutdown();
}

TEST_F(TestClientCursorAPI, cursor_request_applied_without_cursor_motion)
{
    using namespace ::testing;

    struct ChangingCursorClient : NamedCursorClient
    {
        using NamedCursorClient::NamedCursorClient;

        void setup_cursor(MirWindow* window) override
        {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
            auto conf1 = mir_cursor_configuration_from_name(cursor_name.c_str());
            auto conf2 = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
#pragma GCC diagnostic pop

            mir_window_configure_cursor(window, conf1);
            mir_window_configure_cursor(window, conf2);

            mir_cursor_configuration_destroy(conf1);
            mir_cursor_configuration_destroy(conf2);
        }
    };

    client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    ChangingCursorClient client{new_connection(), client_name_1, client_cursor_1};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    InSequence seq;
    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_1)));
    EXPECT_CALL(cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    client.run();

    EXPECT_TRUE(wait.wait_for(timeout));

    expectations_satisfied.wait_for(std::chrono::seconds{5});

    expect_client_shutdown();
}

TEST_F(TestClientCursorAPI, cursor_request_applied_from_buffer_stream)
{
    using namespace ::testing;
    
    static int hotspot_x = 1, hotspot_y = 1;

    struct BufferStreamClient : CursorClient
    {
        using CursorClient::CursorClient;

        void setup_cursor(MirWindow* window) override
        {
            auto stream = mir_connection_create_buffer_stream_sync(
                connection, 24, 24, mir_pixel_format_argb_8888,
                mir_buffer_usage_software);
            auto conf = mir_cursor_configuration_from_buffer_stream(stream, hotspot_x, hotspot_y);

            mir_buffer_stream_swap_buffers_sync(stream);

            mir_window_configure_cursor(window, conf);
            
            mir_cursor_configuration_destroy(conf);            
            
            mir_buffer_stream_swap_buffers_sync(stream);
            mir_buffer_stream_swap_buffers_sync(stream);

            mir_buffer_stream_release_sync(stream);
        }
    };

    client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    BufferStreamClient client{new_connection(), client_name_1};

    {
        InSequence seq;
        EXPECT_CALL(cursor, show(_)).Times(2);
        EXPECT_CALL(cursor, show(_)).Times(1)
            .WillOnce(mt::WakeUp(&expectations_satisfied));
    }

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_))
        .WillRepeatedly(mt::WakeUp(&wait));

    client.run();

    EXPECT_TRUE(wait.wait_for(timeout));

    expectations_satisfied.wait_for(std::chrono::seconds{500});

    expect_client_shutdown();
}

namespace
{
// The nested server fixture we use is using the 'CanonicalWindowManager' which will place
// surfaces in the center...in order to ensure the window appears under the cursor we use
// the fullscreen state.
struct FullscreenDisabledCursorClient : CursorClient
{
    using CursorClient::CursorClient;

    void setup_cursor(MirWindow* window) override
    {
        // Workaround race condition (lp:1525003). I've tried, but I've not
        // found a better way to ensure that the host Mir server is "ready"
        // for the test logic. - alan_g
        std::this_thread::sleep_for(std::chrono::milliseconds(20));

        mir_window_set_state(window, mir_window_state_fullscreen);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        auto conf = mir_cursor_configuration_from_name(mir_disabled_cursor_name);
#pragma GCC diagnostic pop
        mir_window_configure_cursor(window, conf);
        mir_cursor_configuration_destroy(conf);
    }
};

}

TEST_F(TestClientCursorAPI, cursor_passed_through_nested_server)
{
    using namespace ::testing;

    mtf::HeadlessNestedServerRunner nested_mir(new_connection());
    nested_mir.start_server();

    EXPECT_CALL(cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    { // Ensure we finalize the client prior stopping the nested server
        FullscreenDisabledCursorClient client{nested_mir.new_connection(), client_name_1};

        mt::Signal wait;

        EXPECT_CALL(*mock_surface_observer, cursor_image_removed())
            .Times(1)
            .WillOnce(mt::WakeUp(&wait));

        client.run();

        EXPECT_TRUE(wait.wait_for(timeout));

        expectations_satisfied.wait_for(std::chrono::seconds{60});
        expect_client_shutdown();
    }
    
    nested_mir.stop_server();
}
