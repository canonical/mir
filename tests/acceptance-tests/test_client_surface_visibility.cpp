/*
 * Copyright © 2014-2015 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"

#include "mir/scene/session.h"
#include "mir/scene/surface.h"
#include "mir/shell/shell_wrapper.h"
#include "mir/shell/surface_stack.h"

#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/test/signal.h"

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

    mf::SurfaceId create_surface(
        std::shared_ptr<ms::Session> const& session,
        ms::SurfaceCreationParameters const& params,
        std::shared_ptr<mf::EventSink> const& sink) override
    {
        auto const result = msh::ShellWrapper::create_surface(session, params, sink);
        auto const surface = session->surface(result);
        surface->move_to({0, 0});
        surfaces.push_back(surface);
        return result;
    }

    std::shared_ptr<ms::Surface> surface(int index)
    {
        return surfaces[index].lock();
    }

    void raise(int index)
    {
        wrapped->raise_surface(nullptr, surface(index), 0);
    }

private:
    std::vector<std::weak_ptr<ms::Surface>> surfaces;

};

struct MockVisibilityCallback
{
    MOCK_METHOD2(handle, void(MirSurface*,MirSurfaceVisibility));
};

void event_callback(MirSurface* surface, MirEvent const* event, void* ctx)
{
    if (mir_event_get_type(event) != mir_event_type_surface)
        return;
    auto sev = mir_event_get_surface_event(event);
    if (mir_surface_event_get_attribute(sev) != mir_surface_attrib_visibility)
        return;

    auto const mock_callback =
        reinterpret_cast<testing::NiceMock<MockVisibilityCallback>*>(ctx);
    mock_callback->handle(
        surface,
        static_cast<MirSurfaceVisibility>(mir_surface_event_get_attribute_value(sev)));
}

MirSurface* create_surface(MirConnection* connection, geom::Size size,
    testing::NiceMock<MockVisibilityCallback>& mock_callback)
{
    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, size.width.as_int(), size.height.as_int(), mir_pixel_format_bgr_888);
    mir_surface_spec_set_name(spec, "ConnectedClientWithASurfaceFixtureSurface");
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
    mir_surface_spec_set_event_handler(spec, &event_callback, &mock_callback);
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    return surface;
}

void expect_surface_visibility_event_after2(
    MirSurface* surface,
    MockVisibilityCallback& mock_callback,
    MirSurfaceVisibility visibility,
    std::function<void()> const& action)
{
    using namespace testing;

    mt::Signal event_received;

    Mock::VerifyAndClearExpectations(&mock_callback);

    EXPECT_CALL(mock_callback, handle(surface, visibility))
        .WillOnce(DoAll(Invoke([&visibility](MirSurface *s, MirSurfaceVisibility)
            {
                EXPECT_EQ(visibility, mir_surface_get_visibility(s));
            }), mt::WakeUp(&event_received)));

    action();

    event_received.wait_for(std::chrono::seconds{2});

    Mock::VerifyAndClearExpectations(&mock_callback);
}

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

        surface = create_surface(connection, small_size, mock_callback);
        expect_surface_visibility_event_after2(surface, mock_callback,
            mir_surface_visibility_exposed,
                [&]
                {
                    mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
                });
    }

    void TearDown() override
    {
        // Don't call ConnectedClientHeadlessServer::TearDown() - the sequence matters
        mir_surface_release_sync(surface);
        if (second_surface)
            mir_surface_release_sync(second_surface);

        mtf::ConnectedClientHeadlessServer::TearDown();
    }

    void create_larger_surface_on_top()
    {
        second_surface = create_surface(connection, large_size, mock_callback2);
        ASSERT_TRUE(mir_surface_is_valid(second_surface));
    
        shell.lock()->raise(1);

        expect_surface_visibility_event_after2(second_surface, mock_callback2,
            mir_surface_visibility_exposed,
                [&]
                {
                    mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(second_surface));
                });
    }

    std::shared_ptr<ms::Surface> server_surface(size_t index)
    {
        return shell.lock()->surface(index);
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
        MirSurfaceVisibility visibility,
        std::function<void()> const& action)
    {
        expect_surface_visibility_event_after2(surface, mock_callback, visibility, action);
    }


    geom::Size const small_size {640, 480};
    geom::Size const large_size {800, 600};
    MirSurface* surface = nullptr;
    MirSurface* second_surface = nullptr;
    testing::NiceMock<MockVisibilityCallback> mock_callback;
    testing::NiceMock<MockVisibilityCallback> mock_callback2;
    std::weak_ptr<StoringShell> shell;
};

}

TEST_F(MirSurfaceVisibilityEvent, occluded_received_when_surface_goes_off_screen)
{
    expect_surface_visibility_event_after(
        mir_surface_visibility_occluded,
        [this] { move_surface_off_screen(); });
}

TEST_F(MirSurfaceVisibilityEvent, exposed_received_when_surface_reenters_screen)
{
    expect_surface_visibility_event_after(
        mir_surface_visibility_occluded,
        [this] { move_surface_off_screen(); });

    expect_surface_visibility_event_after(
        mir_surface_visibility_exposed,
        [this] { move_surface_into_screen(); });
}

TEST_F(MirSurfaceVisibilityEvent, occluded_received_when_surface_occluded_by_other_surface)
{
    expect_surface_visibility_event_after(
        mir_surface_visibility_occluded,
        [this] { create_larger_surface_on_top(); });
}

TEST_F(MirSurfaceVisibilityEvent, exposed_received_when_surface_raised_over_occluding_surface)
{
    expect_surface_visibility_event_after(
        mir_surface_visibility_occluded,
        [this] { create_larger_surface_on_top(); });

    expect_surface_visibility_event_after(
        mir_surface_visibility_exposed,
        [this] { raise_surface_on_top(); });
}
