/*
 * Copyright © 2014-2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell_wrapper.h"
#include "mir/shell/surface_stack.h"

#include "mir_test_framework/connected_client_with_a_window.h"
#include "mir/test/signal_actions.h"
#include "mir/test/spin_wait.h"

#include <mutex>
#include <condition_variable>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mt = mir::test;
namespace ms = mir::scene;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace geom = mir::geometry;

namespace
{
class StoringShell : public msh::ShellWrapper
{
public:
    StoringShell(std::shared_ptr<msh::Shell> const& wrapped) :
        msh::ShellWrapper{wrapped}
    {}

    auto create_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params,
        std::shared_ptr<mf::EventSink> const& sink) -> std::shared_ptr<ms::Surface> override
    {
        auto const window = msh::ShellWrapper::create_surface(session, params, sink);
        window->move_to({0, 0});
        surfaces.push_back(window);
        return window;
    }

    using msh::ShellWrapper::raise;

    std::shared_ptr<ms::Surface> window(int index)
    {
        return surfaces[index].lock();
    }

    void raise(int index)
    {
        wrapped->raise_surface(nullptr, window(index), 0);
    }

private:
    std::vector<std::weak_ptr<ms::Surface>> surfaces;

};

struct MockVisibilityCallback
{
    MOCK_METHOD2(handle, void(MirWindow*,MirWindowVisibility));
};

void null_event_callback(MirWindow*, MirEvent const*, void*)
{
}

void event_callback(MirWindow* window, MirEvent const* event, void* ctx)
{
    if (mir_event_get_type(event) != mir_event_type_window)
        return;
    auto sev = mir_event_get_window_event(event);
    auto attrib = mir_window_event_get_attribute(sev);
    if (attrib != mir_window_attrib_visibility)
        return;

    auto const mock_callback =
        reinterpret_cast<testing::NiceMock<MockVisibilityCallback>*>(ctx);
    mock_callback->handle(
        window,
        static_cast<MirWindowVisibility>(mir_window_event_get_attribute_value(sev)));
}

MirWindow* create_surface(MirConnection* connection, const char* name, geom::Size size,
    testing::NiceMock<MockVisibilityCallback>& mock_callback)
{
    auto const spec = mir_create_normal_window_spec(
        connection, size.width.as_int(), size.height.as_int());
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_bgr_888);
    mir_window_spec_set_name(spec, name);
    mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
#pragma GCC diagnostic pop
    mir_window_spec_set_event_handler(spec, &event_callback, &mock_callback);
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    return window;
}

struct Surface
{
    Surface(MirConnection* connection, char const* name, geom::Size size) :
        window(create_surface(connection, name, size, callback))
    {
        wait_for_visible_and_focused();
    }

    ~Surface()
    {
        mir_window_set_event_handler(window, null_event_callback, nullptr);
        mir_window_release_sync(window);
    }

    void expect_surface_visibility_event_after(
        MirWindowVisibility visibility,
        std::function<void()> const& action)
    {
        using namespace testing;

        mt::Signal event_received;

        Mock::VerifyAndClearExpectations(&callback);

        EXPECT_CALL(callback, handle(window, visibility))
            .WillOnce(DoAll(Invoke([&visibility](MirWindow *s, MirWindowVisibility)
                {
                    EXPECT_EQ(visibility, mir_window_get_visibility(s));
                }), mt::WakeUp(&event_received)));

        action();

        event_received.wait_for(std::chrono::seconds{2});

        Mock::VerifyAndClearExpectations(&callback);
    }

private:
    void wait_for_visible_and_focused()
    {
        expect_surface_visibility_event_after(
            mir_window_visibility_exposed,
            [this]
            {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
                mir_buffer_stream_swap_buffers_sync(mir_window_get_buffer_stream(window));
#pragma GCC diagnostic pop
            });

        // GMock is behaving strangely, checking expectations after they
        // have been cleared, so we use spin_wait() instead.
        mt::spin_wait_for_condition_or_timeout(
            [this] { return mir_window_get_focus_state(window) == mir_window_focus_state_focused; },
            std::chrono::seconds{2});
    }

    testing::NiceMock<MockVisibilityCallback> callback;
    MirWindow* window;
};

struct MirSurfaceVisibilityEvent : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        server.wrap_shell([&](std::shared_ptr<msh::Shell> const& wrapped)
            {
                auto const result = std::make_shared<StoringShell>(wrapped);
                shell = result;
                return result;
            });

        mtf::ConnectedClientHeadlessServer::SetUp();
        window = std::make_unique<Surface>(connection, "small", small_size);
    }

    void TearDown() override
    {
        window.reset();
        second_surface.reset();
        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    void create_larger_surface_on_top()
    {
        second_surface = std::make_unique<Surface>(connection, "large", large_size);
        shell.lock()->raise(1);
    }

    std::shared_ptr<ms::Surface> server_surface(size_t index)
    {
        return shell.lock()->window(index);
    }

    void move_surface_off_screen()
    {
        server_surface(0)->move_to(geom::Point{10000, 10000});
    }

    void move_surface_into_screen()
    {
        server_surface(0)->move_to(geom::Point{0, 0});
    }

    void raise_surface_on_top()
    {
        shell.lock()->raise(0);
    }

    void expect_surface_visibility_event_after(
        MirWindowVisibility visibility,
        std::function<void()> const& action)
    {
        window->expect_surface_visibility_event_after(visibility, action);
    }

    geom::Size const small_size {640, 480};
    geom::Size const large_size {800, 600};
    std::unique_ptr<Surface> window;
    std::unique_ptr<Surface> second_surface;
    std::weak_ptr<StoringShell> shell;
};

}

TEST_F(MirSurfaceVisibilityEvent, occluded_received_when_surface_goes_off_screen)
{
    expect_surface_visibility_event_after(
        mir_window_visibility_occluded,
        [this] { move_surface_off_screen(); });
}

TEST_F(MirSurfaceVisibilityEvent, exposed_received_when_surface_reenters_screen)
{
    expect_surface_visibility_event_after(
        mir_window_visibility_occluded,
        [this] { move_surface_off_screen(); });

    expect_surface_visibility_event_after(
        mir_window_visibility_exposed,
        [this] { move_surface_into_screen(); });
}

TEST_F(MirSurfaceVisibilityEvent, occluded_received_when_surface_occluded_by_other_surface)
{
    expect_surface_visibility_event_after(
        mir_window_visibility_occluded,
        [this] { create_larger_surface_on_top(); });
}

TEST_F(MirSurfaceVisibilityEvent, exposed_received_when_surface_raised_over_occluding_surface)
{
    expect_surface_visibility_event_after(
        mir_window_visibility_occluded,
        [this] { create_larger_surface_on_top(); });

    expect_surface_visibility_event_after(
        mir_window_visibility_exposed,
        [this] { raise_surface_on_top(); });
}
