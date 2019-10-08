/*
 * Copyright © 2013-2016 Canonical Ltd.
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
 *
 * Authored by: Robert Carr <robert.carr@canonical.com>
 *              Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/graphics/cursor.h"
#include "mir/graphics/cursor_image.h"
#include "mir/input/cursor_images.h"
#include "mir/input/input_device_info.h"
#include "mir/scene/surface_observer.h"
#include "mir/scene/surface.h"

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
#include "mir/test/signal_actions.h"

#include "mir_toolkit/mir_client_library.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mis = mir::input::synthesis;
namespace msh = mir::shell;
namespace msc = mir::scene;
namespace geom = mir::geometry;

namespace mt = mir::test;
namespace mtd = mt::doubles;
namespace mtf = mir_test_framework;

using namespace testing;
using namespace std::chrono_literals;

namespace
{

class MockSurfaceObserver : public msc::SurfaceObserver
{
public:
    MOCK_METHOD3(attrib_changed, void(msc::Surface const*, MirWindowAttrib attrib, int value));
    MOCK_METHOD2(resized_to, void(msc::Surface const*, geom::Size const& size));
    MOCK_METHOD2(moved_to, void(msc::Surface const*, geom::Point const& top_left));
    MOCK_METHOD2(hidden_set_to, void(msc::Surface const*, bool hide));
    MOCK_METHOD3(frame_posted, void(msc::Surface const*, int frames_available, geom::Size const& size));
    MOCK_METHOD2(alpha_set_to, void(msc::Surface const*, float alpha));
    MOCK_METHOD2(orientation_set_to, void(msc::Surface const*, MirOrientation orientation));
    MOCK_METHOD2(transformation_set_to, void(msc::Surface const*, glm::mat4 const& t));
    MOCK_METHOD2(reception_mode_set_to, void(msc::Surface const*, mi::InputReceptionMode mode));
    MOCK_METHOD2(cursor_image_set_to, void(msc::Surface const*, mg::CursorImage const& image));
    MOCK_METHOD1(client_surface_close_requested, void(msc::Surface const*));
    MOCK_METHOD6(keymap_changed, void(
        msc::Surface const*,
        MirInputDeviceId id,
        std::string const& model,
        std::string const& layout,
        std::string const& variant,
        std::string const& options));
    MOCK_METHOD2(renamed, void(msc::Surface const*, char const* name));
    MOCK_METHOD1(cursor_image_removed, void(msc::Surface const*));
    MOCK_METHOD2(placed_relative, void(msc::Surface const*, geom::Rectangle const& placement));
    MOCK_METHOD2(input_consumed, void(msc::Surface const*, MirEvent const*));
    MOCK_METHOD2(start_drag_and_drop, void(msc::Surface const*, std::vector<uint8_t> const& handle));
    MOCK_METHOD2(depth_layer_set_to, void(msc::Surface const*, MirDepthLayer depth_layer));
    MOCK_METHOD2(application_id_set_to, void(msc::Surface const*, std::string const& application_id));
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

struct WindowReadyHandler
{
    static void callback(MirWindow*, MirEvent const* event, void* context)
    {
        auto handler = reinterpret_cast<WindowReadyHandler *>(context);
        handler->handle_event(event);
    }

    void handle_event(MirEvent const* event)
    {
        auto type = mir_event_get_type(event);
        if (type != mir_event_type_window)
            return;

        auto window_event = mir_event_get_window_event(event);
        auto const attrib = mir_window_event_get_attribute(window_event);
        auto const value = mir_window_event_get_attribute_value(window_event);

        if (attrib == mir_window_attrib_visibility && value == mir_window_visibility_exposed)
        {
            window_exposed = true;
        }

        if (attrib == mir_window_attrib_focus && value == mir_window_focus_state_focused)
        {
            window_focused = true;
        }

        if (window_exposed && window_focused)
            window_focused_and_exposed.raise();
    }

    void wait_for_window_to_become_focused_and_exposed()
    {
        if (!window_focused_and_exposed.wait_for(30s))
            throw std::runtime_error("Timed out waiting for window to be focused and exposed");
    }

    bool window_exposed{false};
    bool window_focused{false};
    mir::test::Signal window_focused_and_exposed;
};

MirWindow *make_window(MirConnection *connection, std::string const& client_name)
{
    auto spec = mir_create_normal_window_spec(connection, 1, 1);
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
#pragma GCC diagnostic pop
    mir_window_spec_set_name(spec, client_name.c_str());

    WindowReadyHandler event_handler;
    mir_window_spec_set_event_handler(spec, WindowReadyHandler::callback, &event_handler);

    auto const window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
    event_handler.wait_for_window_to_become_focused_and_exposed();
    mir_window_set_event_handler(window, nullptr, nullptr);

    return window;
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
struct MirSurfaceDeleter
{
    void operator()(MirRenderSurface *surface) { mir_render_surface_release(surface); }
};

using RenderSurface = std::unique_ptr<MirRenderSurface, MirSurfaceDeleter>;
#pragma GCC diagnostic pop

struct CursorClient
{
    CursorClient(std::string const& connect_string, std::string const& client_name)
        : connection(mir_connect_sync(connect_string.c_str(), client_name.c_str())),
          window(make_window(connection, client_name))
    {
    }

    virtual ~CursorClient()
    {
        if (window)
            mir_window_release_sync(window);
        if (connection)
            mir_connection_release(connection);
    }

    RenderSurface make_surface()
    {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
        RenderSurface surface{mir_connection_create_render_surface_sync(connection, 24, 24)};
        return surface;
#pragma GCC diagnostic pop
    }


    virtual void configure_cursor() = 0;

    MirConnection* connection{nullptr};
    MirWindow *window{nullptr};

    int const hotspot_x{1};
    int const hotspot_y{1};

};

struct CursorWindowSpec
{
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    CursorWindowSpec(MirConnection *connection, RenderSurface const& surface, int hotspot_x, int hotspot_y)
    : spec(mir_create_window_spec(connection))
    {
        mir_window_spec_set_cursor_render_surface(spec, surface.get(), hotspot_x, hotspot_y);
    }
#pragma GCC diagnostic pop

    ~CursorWindowSpec()
    {
        mir_window_spec_release(spec);
    }

    void apply(MirWindow *window)
    {
        mir_window_apply_spec(window, spec);
    }
    MirWindowSpec * const spec;
};

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
struct BufferStream
{
    BufferStream(MirConnection *connection)
     : stream_owned{true},
       stream{mir_connection_create_buffer_stream_sync(connection, 24, 24, mir_pixel_format_argb_8888,mir_buffer_usage_software)}
    {}

    BufferStream(RenderSurface const& surface)
     : stream_owned(false),
       stream{mir_render_surface_get_buffer_stream(surface.get(), 24, 24, mir_pixel_format_argb_8888)}
    {}

    ~BufferStream()
    {
        if (stream_owned)
            mir_buffer_stream_release_sync(stream);
    }

    void swap_buffers()
    {
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    bool stream_owned;
    MirBufferStream *stream;
};
#pragma GCC diagnostic pop

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
struct CursorConfiguration
{

    CursorConfiguration(std::string const& name)
     : conf(mir_cursor_configuration_from_name(name.c_str())) {}
    CursorConfiguration(const char * name)
     : conf(mir_cursor_configuration_from_name(name)) {}
    CursorConfiguration(RenderSurface const& surface, int hotspot_x, int hotspot_y)
     : conf(mir_cursor_configuration_from_render_surface(surface.get(), hotspot_x, hotspot_y)) {}
    CursorConfiguration(BufferStream const& stream, int hotspot_x, int hotspot_y)
     : conf(mir_cursor_configuration_from_buffer_stream(stream.stream, hotspot_x, hotspot_y)) {}


    ~CursorConfiguration()
    {
        if (conf)
            mir_cursor_configuration_destroy(conf);
    }

    void apply(MirWindow *window)
    {
        mir_window_configure_cursor(window, conf);
    }

    MirCursorConfiguration * conf;
};
#pragma GCC diagnostic pop

struct DisabledCursorClient : CursorClient
{
    using CursorClient::CursorClient;

    void configure_cursor() override
    {
        CursorConfiguration conf{mir_disabled_cursor_name};
        conf.apply(window);
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

    void configure_cursor() override
    {
        CursorConfiguration conf{cursor_name};
        conf.apply(window);
    }

    std::string const cursor_name;
};

struct ChangingCursorClient : NamedCursorClient
{
    using NamedCursorClient::NamedCursorClient;

    void configure_cursor() override
    {
        CursorConfiguration conf1{cursor_name};
        CursorConfiguration conf2{mir_disabled_cursor_name};

        conf1.apply(window);
        conf2.apply(window);
    }
};

struct BufferStreamClient : CursorClient
{
    using CursorClient::CursorClient;

    void configure_cursor() override
    {
        BufferStream stream{connection};
        CursorConfiguration conf{stream, hotspot_x, hotspot_y};

        stream.swap_buffers();
        conf.apply(window);
        stream.swap_buffers();
        stream.swap_buffers();
    }
};

struct SurfaceCursorConfigClient : CursorClient
{
    using CursorClient::CursorClient;

    void configure_cursor() override
    {
        auto surface = make_surface();
        BufferStream stream{surface};
        CursorConfiguration conf{surface, hotspot_x, hotspot_y};
        stream.swap_buffers();

        conf.apply(window);

        stream.swap_buffers();
        stream.swap_buffers();
    }
};

struct SurfaceCursorClient : CursorClient
{
    using CursorClient::CursorClient;
    void configure_cursor() override
    {
        auto surface = make_surface();
        BufferStream stream{surface};

        stream.swap_buffers();

        CursorWindowSpec spec{connection, surface, hotspot_x, hotspot_y};
        spec.apply(window);

        stream.swap_buffers();
        stream.swap_buffers();
    }
};

struct ClientCursor : mtf::HeadlessInProcessServer
{
    // mtf::add_fake_input_device needs this library to be loaded each test, for the tests
    mtf::TemporaryEnvironmentValue input_lib{"MIR_SERVER_PLATFORM_INPUT_LIB", mtf::server_platform("input-stub.so").c_str()};
    NiceMock<MockCursor> cursor;
    std::shared_ptr<NiceMock<MockSurfaceObserver>> mock_surface_observer =
        std::make_shared<NiceMock<MockSurfaceObserver>>();

    mtf::SurfaceGeometries client_geometries;

    ClientCursor()
    {
        mock_egl.provide_egl_extensions();
        mock_egl.provide_stub_platform_buffer_swapping();

        server.wrap_cursor([this](std::shared_ptr<mg::Cursor> const&) { return mt::fake_shared(cursor); });

        server.override_the_cursor_images([]() { return std::make_shared<NamedCursorImages>(); });

        override_window_management_policy<mtf::DeclarativePlacementWindowManagerPolicy>(client_geometries);

        server.wrap_shell([&, this](auto const& wrapped)
        {
            return std::make_shared<mtf::ObservantShell>(wrapped, mock_surface_observer);
        });
    }

    void expect_client_shutdown()
    {
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

    NiceMock<mtd::MockEGL> mock_egl;
    std::chrono::seconds const timeout{5s};
};

}

// In this set we create a 1x1 client window at the point (1,0). The client requests to disable the cursor
// over this window. Since the cursor starts at (0,0) we when we move the cursor by (1,0) thus causing it
// to enter the bounds of the first window, we should observe it being disabled.
TEST_F(ClientCursor, can_be_disabled)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_removed(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    DisabledCursorClient client{new_connection(), client_name_1};
    client.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));

    EXPECT_CALL(cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));

    expectations_satisfied.wait_for(timeout);

    expect_client_shutdown();
}

TEST_F(ClientCursor, is_restored_when_leaving_surface)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_removed(_))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    DisabledCursorClient client{new_connection(), client_name_1};
    client.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));

    InSequence seq;
    EXPECT_CALL(cursor, hide());
    EXPECT_CALL(cursor, show(DefaultCursorImage()))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(2, 0));

    expectations_satisfied.wait_for(timeout);

    expect_client_shutdown();
}

TEST_F(ClientCursor, is_changed_when_crossing_surface_boundaries)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};
    client_geometries[client_name_2] =
        geom::Rectangle{{2, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_1{new_connection(), client_name_1, client_cursor_1};
    client_1.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));
    wait.reset();

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_2{new_connection(), client_name_2, client_cursor_2};
    client_2.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));

    InSequence seq;
    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_1)));
    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_2)))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));
    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));

    expectations_satisfied.wait_for(timeout);

    expect_client_shutdown();
}

TEST_F(ClientCursor, of_topmost_window_is_applied)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{1, 0}, {1, 1}};
    client_geometries[client_name_2] =
        geom::Rectangle{{1, 0}, {1, 1}};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_1{new_connection(), client_name_1, client_cursor_1};
    client_1.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));
    wait.reset();

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    NamedCursorClient client_2{new_connection(), client_name_2, client_cursor_2};
    client_2.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));

    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_2)))
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    fake_mouse->emit_event(mis::a_pointer_event().with_movement(1, 0));

    expectations_satisfied.wait_for(timeout);

    expect_client_shutdown();
}

TEST_F(ClientCursor, is_applied_without_cursor_motion)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    ChangingCursorClient client{new_connection(), client_name_1, client_cursor_1};

    mt::Signal wait;

    EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
        .Times(1)
        .WillOnce(mt::WakeUp(&wait));

    InSequence seq;
    EXPECT_CALL(cursor, show(CursorNamed(client_cursor_1)));
    EXPECT_CALL(cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    client.configure_cursor();

    EXPECT_TRUE(wait.wait_for(timeout));

    expectations_satisfied.wait_for(timeout);

    expect_client_shutdown();
}

TEST_F(ClientCursor, from_buffer_stream_is_applied)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    BufferStreamClient client{new_connection(), client_name_1};

    {
        InSequence seq;
        EXPECT_CALL(cursor, show(_)).Times(2);
        EXPECT_CALL(cursor, show(_)).Times(1)
            .WillOnce(mt::WakeUp(&expectations_satisfied));
    }

    mt::Signal cursor_image_set;

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _)).Times(2);
        EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
            .WillOnce(mt::WakeUp(&cursor_image_set));
    }

    client.configure_cursor();

    EXPECT_TRUE(cursor_image_set.wait_for(timeout));

    expectations_satisfied.wait_for(60s);

    expect_client_shutdown();
}

TEST_F(ClientCursor, from_a_surface_config_is_applied)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    SurfaceCursorConfigClient client{new_connection(), client_name_1};

    {
        InSequence seq;
        EXPECT_CALL(cursor, show(_)).Times(2);
        EXPECT_CALL(cursor, show(_)).Times(1)
            .WillOnce(mt::WakeUp(&expectations_satisfied));
    }

    mt::Signal cursor_image_set;

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _)).Times(2);
        EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
            .WillOnce(mt::WakeUp(&cursor_image_set));
    }

    client.configure_cursor();

    EXPECT_TRUE(cursor_image_set.wait_for(timeout));

    expectations_satisfied.wait_for(60s);

    expect_client_shutdown();
}

TEST_F(ClientCursor, from_a_surface_is_applied)
{
    client_geometries[client_name_1] =
        geom::Rectangle{{0, 0}, {1, 1}};

    SurfaceCursorClient client{new_connection(), client_name_1};

    {
        InSequence seq;
        EXPECT_CALL(cursor, show(_)).Times(2);
        EXPECT_CALL(cursor, show(_)).Times(1)
            .WillOnce(mt::WakeUp(&expectations_satisfied));
    }

    mt::Signal cursor_image_set;

    {
        InSequence seq;
        EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _)).Times(2);
        EXPECT_CALL(*mock_surface_observer, cursor_image_set_to(_, _))
            .WillOnce(mt::WakeUp(&cursor_image_set));
    }

    client.configure_cursor();

    EXPECT_TRUE(cursor_image_set.wait_for(timeout));

    expectations_satisfied.wait_for(60s);
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

    void configure_cursor() override
    {
        // Workaround race condition (lp:1525003). I've tried, but I've not
        // found a better way to ensure that the host Mir server is "ready"
        // for the test logic. - alan_g
        std::this_thread::sleep_for(20ms);

        mir_window_set_state(window, mir_window_state_fullscreen);
        CursorConfiguration conf{mir_disabled_cursor_name};
        conf.apply(window);
    }
};

}

TEST_F(ClientCursor, passes_through_nested_server)
{
    mtf::HeadlessNestedServerRunner nested_mir(new_connection());
    nested_mir.start_server();

    EXPECT_CALL(cursor, hide())
        .WillOnce(mt::WakeUp(&expectations_satisfied));

    { // Ensure we finalize the client prior stopping the nested server
        FullscreenDisabledCursorClient client{nested_mir.new_connection(), client_name_1};

        mt::Signal wait;

        EXPECT_CALL(*mock_surface_observer, cursor_image_removed(_))
            .Times(1)
            .WillOnce(mt::WakeUp(&wait));

        client.configure_cursor();

        EXPECT_TRUE(wait.wait_for(timeout));

        expectations_satisfied.wait_for(60s);
        expect_client_shutdown();
    }

    nested_mir.stop_server();
}
