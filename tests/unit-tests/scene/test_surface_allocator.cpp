/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "src/server/scene/surface_allocator.h"
#include "mir_test_doubles/mock_buffer_stream.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace
{

struct MockBufferStreamFactory : public ms::BufferStreamFactory
{
    MockBufferStreamFactory()
    {
    }

    MOCK_METHOD1(create_buffer_stream, std::shared_ptr<ms::BufferStream>(mc::BufferProperties const&));
};

struct MockInputChannelFactory : public mi::InputChannelFactory
{
    MOCK_METHOD0(make_input_channel, std::shared_ptr<mi::InputChannel>());
};

struct SurfaceAllocator : public testing::Test;
{
    void SetUp()
    {
        using namespace testing;

        default_size = geom::Size{geom::Width{9}, geom::Height{4}};
        ON_CALL(mock_buffer_stream, stream_size())
            .WillByDefault(Return(default_size));
        ON_CALL(mock_stream_factory, create_buffer_stream(_))
            .WillByDefault(Return(mt::fake_shared(mock_buffer_stream)));
    }

    geom::Size default_size;
    MockBufferStreamFactory mock_stream_factory;
    MockInputChannelFactory mock_input_factory;
    mtd::MockBufferStream mock_buffer_stream;
};
}

TEST_F(SurfaceStack, surface_creation)
{
    msh::SurfaceCreationParameters const& params;

    EXPECT_CALL(buffer_stream_factory, create_buffer_stream(params))
        .Times(1);
    EXPECT_CALL(mock_buffer_stream, stream_size())
        .Times(1);
    EXPECT_CALL(mock_input_factory, make_input_channel())
        .Times(1);

    ms::SurfaceAllocator allocator(mock_stream_factory, mock_input_factory);
    std::function<void()> cb;
    allocator.create_surface(params, cb);
}
