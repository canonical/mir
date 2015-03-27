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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir_test_framework/connected_client_headless_server.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"

#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mtf = mir_test_framework;
namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace
{
MirPixelFormat an_available_format(MirConnection* connection)
{
    using namespace testing;
    MirPixelFormat format{mir_pixel_format_invalid};
    unsigned int valid_formats{0};
    mir_connection_get_available_surface_formats(connection, &format, 1, &valid_formats);
    return format;
}

struct MirClient
{
    MirClient(MirConnection* connection, geom::Rectangle rect) :
        rect(rect),
        surface(mir_surface_create_sync(mir_connection_create_spec_for_normal_surface(
            connection,
            rect.size.width.as_int(),
            rect.size.height.as_int(),
            an_available_format(connection))))
    {
        mir_buffer_stream_swap_buffers_sync(mir_surface_get_buffer_stream(surface));
    }
    ~MirClient()
    {
        mir_surface_release_sync(surface);
    }
    MirClient(MirClient const&) = delete;
    MirClient& operator=(MirClient const&) = delete;
    geom::Rectangle const rect;
    MirSurface* surface;
};

struct Ordering
{
    void note_scene_element_sequence(mc::SceneElementSequence const& sequence)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        rectangles.clear();
        for(auto const& element : sequence)
            rectangles.emplace_back(element->renderable()->screen_position());
        post_count++;
        cv.notify_all();
    }

    template<typename T, typename S>
    bool wait_for_another_post_within(std::chrono::duration<T,S> duration)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        auto count = post_count + 1;
        return cv.wait_for(lk, duration, [this, count]{return (post_count >= count);});
    }

    bool ensure_last_ordering_is_consistent_with(
        std::vector<MirSurfaceArrangement> const& arrangement)
    {
        if (rectangles.size() != arrangement.size())
            return false;
        for(auto i = 0u; i < rectangles.size(); i++)
        {
            if ((rectangles[i].top_left.x.as_int() != arrangement[i].x) ||
                (rectangles[i].top_left.y.as_int() != arrangement[i].y))
                return false;
        }
        return true;
    }
private:
    std::mutex mutex;
    std::condition_variable cv;
    unsigned int post_count{0};
    std::vector<geom::Rectangle> rectangles;
};

struct OrderTrackingDBC : mc::DisplayBufferCompositor
{
    OrderTrackingDBC(
        std::shared_ptr<mc::DisplayBufferCompositor> const& wrapped,
        std::shared_ptr<Ordering> const& ordering) :
        wrapped(wrapped),
        ordering(ordering)
    {
    }

    void composite(mc::SceneElementSequence&& scene_sequence) override
    {
        ordering->note_scene_element_sequence(scene_sequence);
        wrapped->composite(std::move(scene_sequence));
    }

    std::shared_ptr<mc::DisplayBufferCompositor> const wrapped;
    std::shared_ptr<Ordering> const ordering;
};

struct OrderTrackingDBCFactory : mc::DisplayBufferCompositorFactory
{
    OrderTrackingDBCFactory(
        std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped,
        std::shared_ptr<Ordering> const& ordering) :
        wrapped(wrapped),
        ordering(ordering)
    {
    }

    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer& db) override
    {
        return std::make_unique<OrderTrackingDBC>(wrapped->create_compositor_for(db), ordering);
    }

    std::shared_ptr<OrderTrackingDBC> last_dbc{nullptr};
    std::shared_ptr<mc::DisplayBufferCompositorFactory> const wrapped;
    std::shared_ptr<Ordering> const ordering;
};

struct MirSurfaceArrangements : mtf::ConnectedClientHeadlessServer
{
    void SetUp() override
    {
        ordering = std::make_shared<Ordering>();
        server.wrap_display_buffer_compositor_factory(
            [this](std::shared_ptr<mc::DisplayBufferCompositorFactory> const& wrapped)
            {
                order_tracker = std::make_shared<OrderTrackingDBCFactory>(wrapped, ordering);
                return order_tracker;
            });

        ConnectedClientHeadlessServer::SetUp();
        for(auto i = 0; i < num_clients; i++)
        {
            geom::Size size{30 * i + 1, 40* i + 1};
            geom::Point position{i * 2, i * 3};
            clients.emplace_back(std::unique_ptr<MirClient>(new MirClient(connection, geom::Rectangle{position, size})));
        }
    }

    void TearDown() override
    {
        clients.clear();
        ConnectedClientHeadlessServer::TearDown();
    }

    std::shared_ptr<Ordering> ordering;
    std::shared_ptr<OrderTrackingDBCFactory> order_tracker{nullptr};
    int const num_clients{3};
    std::vector<std::unique_ptr<MirClient>> clients;
};
}

TEST_F(MirSurfaceArrangements, occluded_received_when_surface_goes_off_screen)
{
    using namespace std::literals::chrono_literals;
    std::vector<MirSurfaceArrangement> arrangement{clients.size()};
    for(auto i : {1, 0, 2})
        arrangement.emplace_back(MirSurfaceArrangement{
            clients[i]->surface,
            clients[i]->rect.top_left.x.as_int(),
            clients[i]->rect.top_left.y.as_int()});

    EXPECT_TRUE(mir_connection_request_surface_arrangement(connection, arrangement.data(), arrangement.size()));
    EXPECT_TRUE(ordering->wait_for_another_post_within(1s))
         << "timed out waiting for another post";
    EXPECT_TRUE(ordering->ensure_last_ordering_is_consistent_with(arrangement))
         << "surface ordering was not consistent with the client request";
}
