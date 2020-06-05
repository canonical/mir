/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir_toolkit/mir_client_library.h"
#include "mir/graphics/renderable.h"
#include "mir/graphics/cursor.h"
#include "mir/graphics/buffer.h"
#include "mir/compositor/scene_element.h"
#include "buffer_stream_arrangement.h"

#include <stdexcept>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

using namespace std::literals::chrono_literals;
using namespace testing;
namespace mt = mir::test;
namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
MirPixelFormat mt::Stream::an_available_format(MirConnection* connection)
{
    MirPixelFormat format{mir_pixel_format_invalid};
    unsigned int valid_formats{0};
    mir_connection_get_available_surface_formats(connection, &format, 1, &valid_formats);
    return format;
}

mt::RelativeRectangle::RelativeRectangle(
    geom::Displacement const& displacement,
    geom::Size const& logical_size,
    geom::Size const& physical_size) : 
    displacement{displacement},
    logical_size{logical_size},
    physical_size{physical_size}
{
}

bool mt::operator==(mt::RelativeRectangle const& a, mt::RelativeRectangle const& b)
{
    return (a.displacement == b.displacement) &&
        (a.logical_size == b.logical_size) &&
        (a.physical_size == b.physical_size);
}

mt::Stream::Stream(
    geometry::Rectangle position,
    std::function<MirBufferStream*()> const& create_stream) :
    position_{position},
    stream(create_stream())
{
    swap_buffers();
}

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
mt::LegacyStream::LegacyStream(MirConnection* connection,
    geom::Size physical_size,
    geom::Rectangle position) :
    Stream(position,
        [&] {
            return mir_connection_create_buffer_stream_sync(
                connection,
                physical_size.width.as_int(),
                physical_size.height.as_int(),
                an_available_format(connection),
                mir_buffer_usage_software);
            })
{
    swap_buffers();
}

mt::LegacyStream::~LegacyStream()
{
    mir_buffer_stream_release_sync(stream);
}

MirBufferStream* mt::Stream::handle() const
{
    return stream;
}

geom::Point mt::Stream::position()
{
    return position_.top_left;
}

geom::Size mt::Stream::logical_size()
{
    return position_.size;
}

geom::Size mt::Stream::physical_size()
{
    int width = -1;
    int height = -1;
    mir_buffer_stream_get_size(stream, &width, &height);    
    return { width, height };
}

void mt::Stream::set_size(geom::Size size)
{
    mir_buffer_stream_set_size(stream, size.width.as_int(), size.height.as_int());
}

void mt::Stream::swap_buffers()
{
    mir_buffer_stream_swap_buffers_sync(stream);
}

void mt::Ordering::note_scene_element_sequence(mc::SceneElementSequence& sequence)
{
    if (sequence.empty())
        return;

    std::unique_lock<decltype(mutex)> lk(mutex);
    std::vector<mt::RelativeRectangle> position;
    auto first_position = (*sequence.begin())->renderable()->screen_position().top_left;
    for (auto const& element : sequence)
    {
        position.emplace_back(
            element->renderable()->screen_position().top_left - first_position,
            element->renderable()->screen_position().size,
            element->renderable()->buffer()->size());
    }
    positions.push_back(position);
    cv.notify_all();
}

mt::OrderTrackingDBC::OrderTrackingDBC(
    std::shared_ptr<Ordering> const& ordering) :
    ordering(ordering)
{
}

void mt::OrderTrackingDBC::composite(mc::SceneElementSequence&& scene_sequence)
{
    ordering->note_scene_element_sequence(scene_sequence);
}

mt::OrderTrackingDBCFactory::OrderTrackingDBCFactory(
    std::shared_ptr<Ordering> const& ordering) :
    ordering(ordering)
{
}

std::unique_ptr<mc::DisplayBufferCompositor> mt::OrderTrackingDBCFactory::create_compositor_for(
    mg::DisplayBuffer&)
{
    return std::make_unique<OrderTrackingDBC>(ordering);
}

void mt::BufferStreamArrangementBase::SetUp()
{
    ordering = std::make_shared<Ordering>();
    server.override_the_display_buffer_compositor_factory(
        [this]()
        {
            order_tracker = std::make_shared<OrderTrackingDBCFactory>(ordering);
            return order_tracker;
        });

    ConnectedClientWithAWindow::SetUp();
    server.the_cursor()->hide();

    streams.emplace_back(
        std::make_unique<LegacyStream>(connection, surface_size, geom::Rectangle{geom::Point{0,0}, surface_size}));
    int const additional_streams{3};
    for (auto i = 0; i < additional_streams; i++)
    {
        geom::Size size{30 * i + 1, 40* i + 1};
        geom::Point position{i * 2, i * 3};
        streams.emplace_back(std::make_unique<LegacyStream>(connection, size, geom::Rectangle{position, size}));
    }
}

void mt::BufferStreamArrangementBase::TearDown()
{
    streams.clear();
    ConnectedClientWithAWindow::TearDown();
}

typedef mt::BufferStreamArrangementBase BufferStreamArrangement;
TEST_F(BufferStreamArrangement, can_be_specified_when_creating_surface)
{
    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }

    mir_window_release_sync(window);

    auto const spec = mir_create_normal_window_spec(
        connection,
        surface_size.width.as_int(),
        surface_size.height.as_int());
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    mir_window_spec_set_name(spec, "BufferStreamArrangement.can_be_specified_when_creating_surface");
    mir_window_spec_set_buffer_usage(spec, mir_buffer_usage_software);
    mir_window_spec_set_streams(spec, infos.data(), infos.size());

    window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    EXPECT_TRUE(mir_window_is_valid(window)) << mir_window_get_error_message(window);
}

TEST_F(BufferStreamArrangement, arrangements_are_applied)
{
    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }

    auto change_spec = mir_create_window_spec(connection);
    mir_window_spec_set_streams(change_spec, infos.data(), infos.size());
    mir_window_apply_spec(window, change_spec);
    mir_window_spec_release(change_spec);

    std::vector<mt::RelativeRectangle> positions;
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
    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }

    auto spec = mir_create_normal_window_spec(connection, 100, 100);
    mir_window_spec_set_pixel_format(spec, mir_pixel_format_abgr_8888);
    mir_window_spec_set_streams(spec, infos.data(), infos.size());
    auto window = mir_create_window_sync(spec);
    mir_window_spec_release(spec);
    EXPECT_TRUE(mir_window_is_valid(window));
    EXPECT_THAT(mir_window_get_error_message(window), StrEq(""));
}

TEST_F(BufferStreamArrangement, when_non_default_streams_are_set_surface_get_stream_gives_null)
{
    EXPECT_TRUE(mir_buffer_stream_is_valid(mir_window_get_buffer_stream(window)));

    std::vector<MirBufferStreamInfo> infos(streams.size());
    auto i = 0u;
    for (auto const& stream : streams)
    {
        infos[i++] = MirBufferStreamInfo{
            stream->handle(),
            stream->position().x.as_int(),
            stream->position().y.as_int()};
    }
    auto change_spec = mir_create_window_spec(connection);
    mir_window_spec_set_streams(change_spec, infos.data(), infos.size());
    mir_window_apply_spec(window, change_spec);
    mir_window_spec_release(change_spec);

    EXPECT_THAT(mir_window_get_buffer_stream(window), Eq(nullptr));
}
#pragma GCC diagnostic pop
