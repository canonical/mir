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
#include "mir/shell/surface_specification.h"
#include "mir_toolkit/mir_client_library.h"

#include "mir/geometry/rectangle.h"
#include "mir_test_framework/headless_test.h"
#include "mir/test/signal.h"
#include "mir/test/spin_wait.h"

#include "gmock/gmock.h"

#include <mutex>

namespace msh = mir::shell;
namespace mt = mir::test;
namespace mtf = mir_test_framework;

using namespace testing;
using namespace std::chrono_literals;

namespace
{
std::chrono::seconds const max_wait{4};

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
        connection_{mir_connect_sync(connect_string, __PRETTY_FUNCTION__)}
    {
    }

    MockClient(MockClient&& source) :
        connection_{nullptr}
    {
        std::swap(connection_, source.connection_);
    }

    auto create_surface(int output_id) -> SurfaceHandle
    {
        auto const spec = mir_connection_create_spec_for_normal_surface(
                connection_, 800, 600, mir_pixel_format_bgr_888);

        mir_surface_spec_set_fullscreen_on_output(spec, output_id);
        auto const surface = mir_surface_create_sync(spec);
        mir_surface_spec_release(spec);

        mir_surface_set_event_handler(surface, on_surface_event, this);

        return SurfaceHandle{surface};
    };

    void disconnect()
    {
        if (connection_)
            mir_connection_release(connection_);

        connection_ = nullptr;
    }

    MirConnection* connection()
    {
        return connection_;
    }

    ~MockClient() noexcept
    {
        disconnect();
    }

    MOCK_METHOD2(surface_event, void(MirSurface* surface, const MirEvent* event));

private:
    MirConnection* connection_{nullptr};

    static void on_surface_event(MirSurface* surface, const MirEvent* event, void* client_ptr)
    {
        static_cast<MockClient*>(client_ptr)->surface_event(surface, event);
    }
};

struct OverridenSystemCompositorWindowManager : msh::SystemCompositorWindowManager
{
    OverridenSystemCompositorWindowManager(msh::FocusController* focus_controller,
            std::shared_ptr<msh::DisplayLayout> const& display_layout,
            std::shared_ptr<mir::scene::SessionCoordinator> const& session_coordinator) :
        SystemCompositorWindowManager(focus_controller, display_layout, session_coordinator)
    {
    }

    void modify_surface(
        std::shared_ptr<mir::scene::Session> const& /*session*/,
        std::shared_ptr<mir::scene::Surface> const& /*surface*/,
        mir::shell::SurfaceSpecification const& modifications) override
    {
        std::unique_lock<decltype(mutex)> lock(mutex);
        last_mod = modifications;
    }

    std::mutex mutex;
    mir::shell::SurfaceSpecification last_mod;
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
                wm = std::make_shared<OverridenSystemCompositorWindowManager>(
                         focus_controller,
                         server.the_shell_display_layout(),
                         server.the_session_coordinator());

                return wm;
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

    std::shared_ptr<OverridenSystemCompositorWindowManager> wm;
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
    EXPECT_THAT(mir_surface_get_error_message(surface), HasSubstr("An output ID must be specified"));
}

TEST_F(SystemCompositorWindowManager, if_a_surface_posts_client_gets_focus)
{
    auto client = connect_client();

    // Throw away all uninteresting surface events
    EXPECT_CALL(client, surface_event(_, Not(MirFocusEvent(mir_surface_focused)))).Times(AnyNumber());

    mt::Signal signal;

    EXPECT_CALL(client, surface_event(_, MirFocusEvent(mir_surface_focused))).Times(1)
            .WillOnce(InvokeWithoutArgs([&] { signal.raise(); }));

    auto surface = client.create_surface(1);
    surface.post_buffer();

    signal.wait_for(1s);
}

TEST_F(SystemCompositorWindowManager, if_no_surface_posts_client_never_gets_focus)
{
    auto client = connect_client();

    // Throw away all uninteresting surface events
    EXPECT_CALL(client, surface_event(_, Not(MirFocusEvent(mir_surface_focused)))).Times(AnyNumber());

    mt::Signal signal;

    ON_CALL(client, surface_event(_, MirFocusEvent(mir_surface_focused)))
            .WillByDefault(InvokeWithoutArgs([&] { signal.raise(); }));

    auto surface = client.create_surface(1);

    EXPECT_FALSE(signal.wait_for(100ms)) << "Unexpected surface_focused event received";
}

TEST_F(SystemCompositorWindowManager, surface_gets_confine_pointer_set)
{
    auto client = connect_client();

    auto surface = client.create_surface(1);

    MirSurfaceSpec* spec = mir_connection_create_spec_for_changes(client.connection());
    mir_surface_spec_set_pointer_confinement(spec, mir_pointer_confined_to_surface);

    mir_surface_apply_spec(surface, spec);
    mir_surface_spec_release(spec);

    mt::spin_wait_for_condition_or_timeout([this]
    {
        std::unique_lock<decltype(wm->mutex)> lock(wm->mutex);
        return wm->last_mod.confine_pointer.is_set();
    }, std::chrono::seconds{max_wait});
}
