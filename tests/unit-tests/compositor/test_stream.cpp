/*
 * Copyright © Canonical Ltd.
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
 */

#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/fake_shared.h"
#include "mir/compositor/stream.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

using namespace testing;
namespace mf = mir::frontend;
namespace mt = mir::test;
namespace mtd = mir::test::doubles;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace
{
struct Stream : Test
{
    Stream() :
        buffers{
            std::make_shared<mtd::StubBuffer>(initial_size),
            std::make_shared<mtd::StubBuffer>(initial_size),
            std::make_shared<mtd::StubBuffer>(initial_size)}
    {
    }
    
    MOCK_METHOD1(called, void(mg::Buffer&));

    geom::Size initial_size{44,2};
    std::vector<std::shared_ptr<mg::Buffer>> buffers;
    MirPixelFormat construction_format{mir_pixel_format_rgb_565};
    mc::Stream stream{
        initial_size, construction_format};
};
}

TEST_F(Stream, tracks_has_buffer)
{
    EXPECT_FALSE(stream.has_submitted_buffer());
    stream.submit_buffer(
            buffers[0],
            buffers[0]->size(),
            {{0, 0}, geom::SizeD{buffers[0]->size()}} );
    EXPECT_TRUE(stream.has_submitted_buffer());
}

TEST_F(Stream, calls_frame_callback_after_scheduling_on_submissions)
{
    int frame_count{0};
    stream.set_frame_posted_callback([&frame_count](auto) { ++frame_count;});
    stream.submit_buffer(
            buffers[0],
            buffers[0]->size(),
            {{0, 0}, geom::SizeD{buffers[0]->size()}} );
    stream.set_frame_posted_callback([](auto) {});
    stream.submit_buffer(
            buffers[0],
            buffers[0]->size(),
            {{0, 0}, geom::SizeD{buffers[0]->size()}} );
    EXPECT_THAT(frame_count, Eq(1));
}

TEST_F(Stream, frame_callback_is_called_without_scheduling_lock)
{
    stream.set_frame_posted_callback(
        [this](auto)
        {
            EXPECT_TRUE(stream.has_submitted_buffer());
        });
    stream.submit_buffer(
            buffers[0],
            buffers[0]->size(),
            {{0, 0}, geom::SizeD{buffers[0]->size()}} );
}

TEST_F(Stream, throws_on_nullptr_submissions)
{
    stream.set_frame_posted_callback([](auto) { FAIL() << "frame-posted should not be called on null buffer"; });
    EXPECT_THROW({
        stream.submit_buffer(
                nullptr,
                buffers[0]->size(),
                {{0, 0}, geom::SizeD{buffers[0]->size()}} );
    }, std::invalid_argument);
    EXPECT_FALSE(stream.has_submitted_buffer());
}

//it doesnt quite make sense that the stream has a size, esp given that there could be different-sized buffers
//in the stream, and the surface has the onscreen size info
TEST_F(Stream, reports_size)
{
    geom::Size new_size{333,139};
    auto new_size_buffer = std::make_shared<mtd::StubBuffer>(new_size);
    EXPECT_THAT(stream.stream_size(), Eq(initial_size));
    stream.submit_buffer(
            new_size_buffer,
            new_size_buffer->size(),
            {{0, 0}, geom::SizeD{new_size_buffer->size()}} );
    EXPECT_THAT(stream.stream_size(), Eq(new_size));
}

//Likewise, no reason buffers couldn't all be a different pixel format
TEST_F(Stream, reports_format)
{
    EXPECT_THAT(stream.pixel_format(), Eq(construction_format));
}

TEST_F(Stream, stream_size_scaled)
{
    stream.submit_buffer(
            buffers[0],
            buffers[0]->size(),
            {{0, 0}, geom::SizeD{buffers[0]->size()}} );
    stream.set_scale(2.0f);
    ASSERT_THAT(stream.stream_size(), Eq(initial_size / 2));
}

TEST_F(Stream, stream_remembers_scale_when_buffer_added)
{
    stream.set_scale(2.0f);
    stream.submit_buffer(
            buffers[0],
            buffers[0]->size(),
            {{0, 0}, geom::SizeD{buffers[0]->size()}} );
    ASSERT_THAT(stream.stream_size(), Eq(initial_size / 2));
}
