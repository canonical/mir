/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/swapper_factory.h"
#include "mir/compositor/buffer_basic.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
static geom::Size const buf_size{geom::Width{100}, geom::Height{121}};
static geom::PixelFormat const buf_pixel_format{geom::PixelFormat::xbgr_8888};

class MockGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
public:
    MockGraphicBufferAllocator()
    {
        using namespace testing;
        mc::BufferProperties properties(buf_size, buf_pixel_format, mc::BufferUsage::hardware);
        ON_CALL(*this, alloc_buffer(_))
            .WillByDefault(Return(std::make_shared<mtd::StubBuffer>(properties)));
    }
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mc::Buffer>(mc::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats, std::vector<geom::PixelFormat>());

    ~MockGraphicBufferAllocator() noexcept {}
};

struct SwapperFactoryTest : testing::Test
{
    SwapperFactoryTest()
        : stub_allocator{std::make_shared<testing::NiceMock<MockGraphicBufferAllocator>>()},
          size{geom::Width{10},geom::Height{20}},
          pf{geom::PixelFormat::abgr_8888},
          usage{mc::BufferUsage::hardware},
          properties{size, pf, usage}

    {
    }

    std::shared_ptr<MockGraphicBufferAllocator> const stub_allocator;
    geom::Size const size;
    geom::PixelFormat const pf;
    mc::BufferUsage const usage;
    mc::BufferProperties const properties;
};
}

/* default number of buffers is 2 */
TEST_F(SwapperFactoryTest, create_swapper_uses_default_number_of_buffers)
{
    using namespace testing;

    mc::SwapperFactory strategy{stub_allocator};
    mc::BufferProperties actual_properties;

    int default_num_of_buffers = 2;
    EXPECT_CALL(*stub_allocator, alloc_buffer(_))
        .Times(default_num_of_buffers);
    auto swapper = strategy.create_swapper(actual_properties, properties);
}

TEST_F(SwapperFactoryTest, create_swapper_with_two_makes_double_buffer)
{
    using namespace testing;

    mc::BufferProperties actual_properties;

    int num_of_buffers = 2;
    mc::SwapperFactory strategy{stub_allocator, num_of_buffers};
    EXPECT_CALL(*stub_allocator, alloc_buffer(_))
        .Times(num_of_buffers);
    auto swapper = strategy.create_swapper(actual_properties, properties);
}

TEST_F(SwapperFactoryTest, create_swapper_with_three_makes_triple_buffer)
{
    using namespace testing;

    mc::BufferProperties actual_properties;

    int num_of_buffers = 3;
    mc::SwapperFactory strategy{stub_allocator, num_of_buffers};
    EXPECT_CALL(*stub_allocator, alloc_buffer(_))
        .Times(num_of_buffers);
    auto swapper = strategy.create_swapper(actual_properties, properties);
}

TEST_F(SwapperFactoryTest, create_swapper_returns_actual_properties_from_buffer)
{
    mc::SwapperFactory strategy{stub_allocator};
    mc::BufferProperties actual_properties;

    auto swapper = strategy.create_swapper(actual_properties, properties);

    EXPECT_EQ(buf_size, actual_properties.size);
    EXPECT_EQ(buf_pixel_format, actual_properties.format);
    EXPECT_EQ(usage, actual_properties.usage);
}
