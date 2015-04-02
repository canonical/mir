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
#include "mir_test_framework/connected_client_with_a_surface.h"
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

struct Stream
{
    Stream(MirBufferStream* stream, geom::Point pt) :
        stream(stream),
        pos{pt},
        needs_release{false}
    {
    }

    Stream(MirConnection* connection, geom::Rectangle rect) :
        stream(mir_connection_create_buffer_stream_sync(
            connection,
            rect.size.width.as_int(),
            rect.size.height.as_int(),
            an_available_format(connection),
            mir_buffer_usage_hardware)),
        pos{rect.top_left},
        needs_release{true}
    {
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    ~Stream()
    {
        if (needs_release)
            mir_buffer_stream_release_sync(stream);
    }

    operator MirBufferStream*() const
    {
        return stream;
    }

    geom::Point position()
    {
        return pos;
    }

    Stream(Stream const&) = delete;
    Stream& operator=(Stream const&) = delete;
private:
    MirBufferStream* stream;
    geom::Point const pos;
    bool const needs_release;
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
        std::vector<MirRectangle> const& arrangement)
    {
        if (rectangles.size() != arrangement.size())
            return false;
        for(auto i = 0u; i < rectangles.size(); i++)
        {
            if ((rectangles[i].top_left.x.as_int() != arrangement[i].left) ||
                (rectangles[i].top_left.y.as_int() != arrangement[i].top))
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

struct BufferStreamArrangement : mtf::ConnectedClientWithASurface
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

        ConnectedClientWithASurface::SetUp();

        primary_stream = std::unique_ptr<Stream>(
            new Stream(mir_surface_get_buffer_stream(surface), geom::Point{0,0}));

        int const additional_streams{3};
        for(auto i = 0; i < additional_streams; i++)
        {
            geom::Size size{30 * i + 1, 40* i + 1};
            geom::Point position{i * 2, i * 3};
            streams.emplace_back(std::unique_ptr<Stream>(
                new Stream(connection, geom::Rectangle{position, size})));
        }
    }

    void TearDown() override
    {
        streams.clear();
        ConnectedClientWithASurface::TearDown();
    }

    std::shared_ptr<Ordering> ordering;
    std::shared_ptr<OrderTrackingDBCFactory> order_tracker{nullptr};
    std::unique_ptr<Stream> primary_stream;
    std::vector<std::unique_ptr<Stream>> streams;
};
}

TEST_F(BufferStreamArrangement, arrangements_are_applied)
{
    using namespace testing;
    MirBufferStream* above_stream = *primary_stream;
    auto change_spec = mir_surface_begin_changes(surface);
    for(auto& stream : streams)
    {
        EXPECT_TRUE(mir_surface_spec_place_buffer_stream_below(change_spec, *stream, above_stream));
        EXPECT_TRUE(mir_surface_spec_place_buffer_stream_position(
            change_spec, *stream, stream->position().x.as_int(), stream->position().y.as_int()));
        above_stream = *stream;
    }
    mir_wait_for(mir_surface_spec_commit_changes(change_spec));
    mir_surface_spec_release(change_spec);

    unsigned int num_streams = mir_surface_get_maximum_number_of_streams(surface);
    ASSERT_THAT(num_streams, Eq(streams.size() + 1));
    std::vector<MirBufferStream*> buffer_streams{num_streams};
    std::vector<MirRectangle> positions{num_streams};
    mir_surface_get_streams(surface, buffer_streams.data(), positions.data(), num_streams);
    for(auto i = 0u; i < num_streams; ++i)
    {
        geom::Point pt{positions[i].top, positions[i].left};
        if (i == 0u)
        {
            EXPECT_THAT(buffer_streams[i], Eq(static_cast<MirBufferStream*>(*primary_stream)));
            EXPECT_THAT(pt, Eq(primary_stream->position()));
        }
        else
        {
            EXPECT_THAT(buffer_streams[i], Eq(static_cast<MirBufferStream*>(*streams[i-1])));
            EXPECT_THAT(pt, Eq(streams[i-1]->position()));
        }
    }

    //check that the compositor rendered correctly
    using namespace std::literals::chrono_literals;
    EXPECT_TRUE(ordering->wait_for_another_post_within(1s))
         << "timed out waiting for another post";
    EXPECT_TRUE(ordering->ensure_last_ordering_is_consistent_with(positions))
         << "surface ordering was not consistent with the client request";
}
