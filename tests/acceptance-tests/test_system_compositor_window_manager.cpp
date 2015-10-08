/*
 * Copyright © 2015 Canonical Ltd.
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
 */

#include "mir/shell/system_compositor_window_manager.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir/geometry/rectangle.h"
#include "mir_test_framework/headless_test.h"
#include "mir/test/signal.h"

#include "gmock/gmock.h"

namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
class SurfaceHandle
{
public:
    explicit SurfaceHandle(MirSurface* surface) : surface{surface} {}
    ~SurfaceHandle() { if (surface) mir_surface_release_sync(surface); }

    operator MirSurface*() const { return surface; }

    void post_buffer()
    {
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }

    SurfaceHandle(SurfaceHandle const&& that) : surface{that.surface} { surface = nullptr; }
private:
    SurfaceHandle(SurfaceHandle const&) = delete;
    MirSurface* surface;
};

struct MockClient
{
    explicit MockClient(char const* connect_string) :
        connection{mir_connect_sync(connect_string, __PRETTY_FUNCTION__)}
    {
    }

    MockClient(MockClient&& source) :
        connection{nullptr}
    {
        std::swap(connection, source.connection);
    }

    auto create_surface(int output_id) -> SurfaceHandle
    {
        auto const spec = mir_connection_create_spec_for_normal_surface(
                connection, 800, 600, mir_pixel_format_bgr_888);

        mir_surface_spec_set_fullscreen_on_output(spec, output_id);
        auto const surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        mir_surface_set_event_handler(surface, on_surface_event, this);

        return SurfaceHandle{surface};
    };

    void disconnect()
    {
        if (connection)
            mir_connection_release(connection);

        connection = nullptr;
    }

    ~MockClient() noexcept
    {
        disconnect();
    }

    MOCK_METHOD2(surface_event, void(MirSurface* surface, const MirEvent* event));

private:
    MirConnection* connection{nullptr};

    static void on_surface_event(MirSurface* surface, const MirEvent* event, void* client_ptr)
    {
        static_cast<MockClient*>(client_ptr)->surface_event(surface, event);
    }
};

struct SystemCompositorWindowManager : mtf::HeadlessTest
{
    void SetUp() override
    {
        add_to_environment("MIR_SERVER_NO_FILE", "");

        initial_display_layout({{{0, 0}, { 640,  480}}, {{480, 0}, {1920, 1080}}});

        server.override_the_window_manager_builder(
            [this](msh::FocusController* focus_controller)
            {
                return std::make_shared<msh::SystemCompositorWindowManager>(
                    focus_controller,
                    server.the_shell_display_layout(),
                    server.the_session_coordinator());
            });

        start_server();
    }

    void TearDown() override
    {
        stop_server();
    }

    MockClient connect_client()
    {
        return MockClient(new_connection().c_str());
    }
};

MATCHER_P(MirFocusEvent, expected, "")
{
    if (mir_event_get_type(arg) != mir_event_type_surface)
        return false;

    auto surface_event = mir_event_get_surface_event(arg);
    auto attrib = mir_surface_event_get_attribute(surface_event);
    auto value = mir_surface_event_get_attribute_value(surface_event);

    return (attrib == mir_surface_attrib_focus)
        && (value == expected);
}
}

TEST_F(SystemCompositorWindowManager, when_output_is_valid_surfaces_creation_succeeds)
{
    auto client = connect_client();

    auto surface1 = client.create_surface(1);
    auto surface2 = client.create_surface(2);

    EXPECT_TRUE(mir_surface_is_valid(surface1));
    EXPECT_TRUE(mir_surface_is_valid(surface2));
}

TEST_F(SystemCompositorWindowManager, when_output_ID_not_specified_surfaces_creation_fails)
{
    auto client = connect_client();

    auto surface = client.create_surface(0);

    EXPECT_FALSE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), HasSubstr("Failed to place surface"));
}

TEST_F(SystemCompositorWindowManager, if_a_surface_posts_client_gets_focus)
{
    auto client = connect_client();

    // Throw away all uninteresting surface events
    EXPECT_CALL(client, surface_event(_, Not(MirFocusEvent(mir_surface_focused)))).Times(AnyNumber());

    auto surface = client.create_surface(1);

    mt::Signal signal;

    EXPECT_CALL(client, surface_event(_, MirFocusEvent(mir_surface_focused))).Times(1)
            .WillOnce(InvokeWithoutArgs([&] { signal.raise(); }));

    surface.post_buffer();

    signal.wait_for(1s);
}

TEST_F(SystemCompositorWindowManager, if_no_surface_posts_client_never_gets_focus)
{
    auto client = connect_client();
    auto surface = client.create_surface(1);

    mt::Signal signal;

    ON_CALL(client, surface_event(_, MirFocusEvent(mir_surface_focused)))
            .WillByDefault(InvokeWithoutArgs([&] { signal.raise(); }));

    EXPECT_FALSE(signal.wait_for(100ms)) << "Unexpected surface_focused event received";
}
