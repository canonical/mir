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
#include "mir_toolkit/mir_presentation_chain.h"
#include "mir_test_framework/connected_client_with_a_surface.h"
#include "mir/compositor/display_buffer_compositor.h"
#include "mir/compositor/display_buffer_compositor_factory.h"
#include "mir/compositor/scene_element.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/buffer.h"
#include "mir/geometry/displacement.h"

#include <mutex>
#include <condition_variable>
#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std::literals::chrono_literals;
namespace mtf = mir_test_framework;
namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace
{

struct RelativeRectangle
{
    RelativeRectangle() = default;

    RelativeRectangle(
        geom::Displacement const& displacement,
        geom::Size const& logical_size,
        geom::Size const& physical_size)
        : displacement{displacement}, logical_size{logical_size}, physical_size{physical_size}
    {
    }

    geom::Displacement displacement;
    geom::Size logical_size;
    geom::Size physical_size;
};
bool operator==(RelativeRectangle const& a, RelativeRectangle const& b)
{
    return (a.displacement == b.displacement) &&
        (a.logical_size == b.logical_size) &&
        (a.physical_size == b.physical_size);
}

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
    Stream(MirConnection* connection,
        geom::Size physical_size,
        geom::Rectangle position) :
        stream(mir_connection_create_buffer_stream_sync(
            connection,
            physical_size.width.as_int(),
            physical_size.height.as_int(),
            an_available_format(connection),
            mir_buffer_usage_hardware)),
        position_{position},
        physical_size_{physical_size},
        needs_release{true}
    {
        swap_buffers();
    }

    ~Stream()
    {
        if (needs_release)
            mir_buffer_stream_release_sync(stream);
    }

    MirBufferStream* handle() const
    {
        return stream;
    }

    geom::Point position()
    {
        return position_.top_left;
    }

    geom::Size logical_size()
    {
        return position_.size;
    }

    geom::Size physical_size()
    {
        return physical_size_;
    }

    void set_size(geom::Size size)
    {
        mir_buffer_stream_set_size(stream, size.width.as_int(), size.height.as_int());
        physical_size_ = size;
    }

    void swap_buffers()
    {
        mir_buffer_stream_swap_buffers_sync(stream);
    }

    Stream(Stream const&) = delete;
    Stream& operator=(Stream const&) = delete;
private:
    MirBufferStream* stream;
    geom::Rectangle const position_;
    geom::Size physical_size_;
    bool const needs_release;
};

struct Ordering
{
    void note_scene_element_sequence(mc::SceneElementSequence& sequence)
    {
        if (sequence.empty())
            return;

        std::unique_lock<decltype(mutex)> lk(mutex);
        std::vector<RelativeRectangle> position;
        auto first_position = (*sequence.begin())->renderable()->screen_position().top_left;
    printf("------------>\n");
        for (auto const& element : sequence)
        {
            printf("LOGICAL %i %i vs Phys %i %i\n",
                element->renderable()->screen_position().size.width.as_int(),
                element->renderable()->screen_position().size.height.as_int(),
                element->renderable()->buffer()->size().width.as_int(),
                element->renderable()->buffer()->size().height.as_int());

            position.emplace_back(
                element->renderable()->screen_position().top_left - first_position,
                element->renderable()->screen_position().size,
                element->renderable()->buffer()->size());
        }
    printf("------------>\n");
        positions.push_back(position);
        cv.notify_all();
    }

    template<typename T, typename S>
    bool wait_for_positions_within(
        std::vector<RelativeRectangle> const& awaited_positions,
        std::chrono::duration<T,S> duration)
    {
        std::unique_lock<decltype(mutex)> lk(mutex);
        return cv.wait_for(lk, duration, [this, awaited_positions] {
            for (auto& position : positions)
                if (position == awaited_positions) return true;
            positions.clear();
            return false;
        });
    }

private:
    std::mutex mutex;
    std::condition_variable cv;
    std::vector<std::vector<RelativeRectangle>> positions;
};

struct OrderTrackingDBC : mc::DisplayBufferCompositor
{
    OrderTrackingDBC(
        std::shared_ptr<Ordering> const& ordering) :
        ordering(ordering)
    {
    }

    void composite(mc::SceneElementSequence&& scene_sequence) override
    {
        ordering->note_scene_element_sequence(scene_sequence);
    }

    std::shared_ptr<Ordering> const ordering;
};

struct OrderTrackingDBCFactory : mc::DisplayBufferCompositorFactory
{
    OrderTrackingDBCFactory(
        std::shared_ptr<Ordering> const& ordering) :
        ordering(ordering)
    {
    }

    std::unique_ptr<mc::DisplayBufferCompositor> create_compositor_for(mg::DisplayBuffer&) override
    {
        return std::make_unique<OrderTrackingDBC>(ordering);
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
        server.override_the_display_buffer_compositor_factory(
            [this]()
            {
                order_tracker = std::make_shared<OrderTrackingDBCFactory>(ordering);
                return order_tracker;
            });

        ConnectedClientWithASurface::SetUp();
        server.the_cursor()->hide();

        streams.emplace_back(
            std::make_unique<Stream>(connection, surface_size, geom::Rectangle{geom::Point{0,0}, surface_size}));
        int const additional_streams{3};
        for (auto i = 0; i < additional_streams; i++)
        {
            geom::Size size{30 * i + 1, 40* i + 1};
            geom::Point position{i * 2, i * 3};
            streams.emplace_back(std::make_unique<Stream>(connection, size, geom::Rectangle{position, size}));
        }
    }

    void TearDown() override
    {
        streams.clear();
        ConnectedClientWithASurface::TearDown();
    }

    std::shared_ptr<Ordering> ordering;
    std::shared_ptr<OrderTrackingDBCFactory> order_tracker{nullptr};
    std::vector<std::unique_ptr<Stream>> streams;
};
}

TEST_F(BufferStreamArrangement, can_be_specified_when_creating_surface)
{
    using namespace testing;
    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }

    mir_surface_release_sync(surface);

    auto const spec = mir_connection_create_spec_for_normal_surface(
        connection, surface_size.width.as_int(), surface_size.height.as_int(), mir_pixel_format_abgr_8888);
    mir_surface_spec_set_name(spec, "BufferStreamArrangement.can_be_specified_when_creating_surface");
    mir_surface_spec_set_buffer_usage(spec, mir_buffer_usage_hardware);
    mir_surface_spec_set_streams(spec, infos.data(), infos.size());

    surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    EXPECT_TRUE(mir_surface_is_valid(surface)) << mir_surface_get_error_message(surface);
}

TEST_F(BufferStreamArrangement, arrangements_are_applied)
{
    using namespace testing;
    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }

    auto change_spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_set_streams(change_spec, infos.data(), infos.size());
    mir_surface_apply_spec(surface, change_spec);
    mir_surface_spec_release(change_spec);

    std::vector<RelativeRectangle> positions;
    i = 0;
    for (auto& info : infos)
    {
        positions.emplace_back(
            geom::Displacement{info.displacement_x, info.displacement_y},
            streams[i]->logical_size(), streams[i]->physical_size());
        i++;
    }

    //check that the compositor rendered correctly
    EXPECT_TRUE(ordering->wait_for_positions_within(positions, 5s))
         << "timed out waiting to see the compositor post the streams in the right arrangement";
}

//LP: #1577967
TEST_F(BufferStreamArrangement, surfaces_can_start_with_non_default_stream)
{
    using namespace testing;
    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }

    auto spec = mir_connection_create_spec_for_normal_surface(
        connection, 100, 100, mir_pixel_format_abgr_8888);
    mir_surface_spec_set_streams(spec, infos.data(), infos.size());
    auto surface = mir_surface_create_sync(spec);
    mir_surface_spec_release(spec);
    EXPECT_TRUE(mir_surface_is_valid(surface));
    EXPECT_THAT(mir_surface_get_error_message(surface), StrEq(""));
}

TEST_F(BufferStreamArrangement, when_non_default_streams_are_set_surface_get_stream_gives_null)
{
    using namespace testing;
    EXPECT_TRUE(mir_buffer_stream_is_valid(mir_surface_get_buffer_stream(surface)));

    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }
    auto change_spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_set_streams(change_spec, infos.data(), infos.size());
    mir_surface_apply_spec(surface, change_spec);
    mir_surface_spec_release(change_spec);

    EXPECT_THAT(mir_surface_get_buffer_stream(surface), Eq(nullptr));
}

TEST_F(BufferStreamArrangement, can_set_stream_logical_and_physical_size)
{
    using namespace testing;

    geom::Size logical_size { 233, 111 };
    geom::Size physical_size { 133, 222 };
    Stream stream(connection, physical_size, { {0, 0}, logical_size } );

    auto change_spec = mir_connection_create_spec_for_changes(connection);
    mir_surface_spec_add_buffer_stream(change_spec, 0, 0, logical_size.width.as_int(), logical_size.height.as_int(), stream.handle());
    mir_surface_apply_spec(surface, change_spec);
    mir_surface_spec_release(change_spec);
/*
    streams.back()->set_size(new_size);
    streams.back()->swap_buffers();
    MirNativeBuffer* native_buffer = nullptr;
    mir_buffer_stream_get_current_buffer(streams.back()->handle(), &native_buffer);
    streams.back()->swap_buffers();
    printf("DONE SWAPPING\n");
*/

    std::vector<RelativeRectangle> positions { { {0,0}, logical_size, physical_size } };
    EXPECT_TRUE(ordering->wait_for_positions_within(positions, 5s))
         << "timed out waiting to see the compositor post the streams in the right arrangement";
}
