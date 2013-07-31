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
#include "mir/graphics/buffer_basic.h"
#include "mir/graphics/buffer_properties.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/graphics/graphic_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"
#include "src/server/compositor/buffer_bundle.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
static geom::Size const buf_size{100, 121};
static geom::PixelFormat const buf_pixel_format{geom::PixelFormat::xbgr_8888};

class MockGraphicBufferAllocator : public mg::GraphicBufferAllocator
{
public:
    MockGraphicBufferAllocator()
    {
        using namespace testing;
        mg::BufferProperties properties(buf_size, buf_pixel_format, mg::BufferUsage::hardware);
        ON_CALL(*this, alloc_buffer(_))
            .WillByDefault(Return(std::make_shared<mtd::StubBuffer>(properties)));
    }
    MOCK_METHOD1(alloc_buffer, std::shared_ptr<mg::Buffer>(mg::BufferProperties const&));
    MOCK_METHOD0(supported_pixel_formats, std::vector<geom::PixelFormat>());

    ~MockGraphicBufferAllocator() noexcept {}
};

struct SwapperFactoryTest : testing::Test
{
    SwapperFactoryTest()
        : mock_buffer_allocator{std::make_shared<testing::NiceMock<MockGraphicBufferAllocator>>()},
          size{10, 20},
          pf{geom::PixelFormat::abgr_8888},
          usage{mg::BufferUsage::hardware},
          properties{size, pf, usage}

    {
    }

    std::shared_ptr<MockGraphicBufferAllocator> const mock_buffer_allocator;
    geom::Size const size;
    geom::PixelFormat const pf;
    mg::BufferUsage const usage;
    mg::BufferProperties const properties;
};
}

/* default number of buffers is 2 */
TEST_F(SwapperFactoryTest, create_sync_uses_default_number_of_buffers)
{
    using namespace testing;

    mg::BufferProperties actual_properties;
    int default_num_of_buffers = 2;
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(properties))
        .Times(default_num_of_buffers);

    mc::SwapperFactory strategy(mock_buffer_allocator);
    auto swapper = strategy.create_swapper_new_buffers(
        actual_properties, properties, mc::SwapperType::synchronous);
}

TEST_F(SwapperFactoryTest, create_sync_with_two_makes_double_buffer)
{
    using namespace testing;

    int num_of_buffers = 2;
    mg::BufferProperties actual_properties;

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(properties))
        .Times(num_of_buffers);

    mc::SwapperFactory strategy(mock_buffer_allocator, num_of_buffers);
    auto swapper = strategy.create_swapper_new_buffers(
        actual_properties, properties, mc::SwapperType::synchronous);
}

TEST_F(SwapperFactoryTest, create_sync_with_three_makes_triple_buffer)
{
    using namespace testing;

    int num_of_buffers = 3;
    mg::BufferProperties actual_properties;

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(properties))
        .Times(num_of_buffers);

    mc::SwapperFactory strategy(mock_buffer_allocator, num_of_buffers);
    auto swapper = strategy.create_swapper_new_buffers(
        actual_properties, properties, mc::SwapperType::synchronous);
}

TEST_F(SwapperFactoryTest, create_async_ignores_preference)
{
    using namespace testing;

    mg::BufferProperties actual_properties;

    int num_of_buffers = 3;
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(properties))
        .Times(num_of_buffers);

    mc::SwapperFactory strategy(mock_buffer_allocator);
    auto swapper = strategy.create_swapper_new_buffers(
        actual_properties, properties, mc::SwapperType::framedropping);
}

TEST_F(SwapperFactoryTest, creation_returns_new_parameters)
{
    mg::BufferProperties actual1, actual2;
    mc::SwapperFactory strategy(mock_buffer_allocator);
    strategy.create_swapper_new_buffers(actual1, properties, mc::SwapperType::synchronous);

    EXPECT_EQ(buf_size, actual1.size);
    EXPECT_EQ(buf_pixel_format, actual1.format);
    EXPECT_EQ(usage, actual1.usage);
}

TEST_F(SwapperFactoryTest, create_async_reuse)
{
    using namespace testing;

    mg::BufferProperties properties;
    std::vector<std::shared_ptr<mg::Buffer>> list {};
    size_t size = 3;

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(properties))
        .Times(0);

    mc::SwapperFactory strategy(mock_buffer_allocator);
    auto swapper = strategy.create_swapper_reuse_buffers(properties, list, size, mc::SwapperType::framedropping);
}

TEST_F(SwapperFactoryTest, create_sync_reuse)
{
    using namespace testing;

    mg::BufferProperties properties;
    std::vector<std::shared_ptr<mg::Buffer>> list;
    size_t size = 3;

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(properties))
        .Times(0);

    mc::SwapperFactory strategy(mock_buffer_allocator, 3);
    auto swapper = strategy.create_swapper_reuse_buffers(properties, list, size, mc::SwapperType::synchronous);
}

TEST_F(SwapperFactoryTest, reuse_drop_unneeded_buffer)
{
    using namespace testing;

    mc::SwapperFactory strategy(mock_buffer_allocator, 2);

    auto buffer = std::make_shared<mtd::StubBuffer>();
    {
        size_t size = 3;
        std::vector<std::shared_ptr<mg::Buffer>> list{buffer};

        auto swapper = strategy.create_swapper_reuse_buffers(
            properties, list, size, mc::SwapperType::synchronous);
    }
    EXPECT_EQ(1, buffer.use_count());
}

TEST_F(SwapperFactoryTest, reuse_drop_unneeded_buffer_error)
{
    using namespace testing;

    mc::SwapperFactory strategy(mock_buffer_allocator, 2);

    size_t size = 3;
    std::vector<std::shared_ptr<mg::Buffer>> list{};

    EXPECT_THROW({
        strategy.create_swapper_reuse_buffers(
            properties, list, size, mc::SwapperType::synchronous);
    }, std::logic_error);
}

TEST_F(SwapperFactoryTest, reuse_alloc_additional_buffer_for_framedropping)
{
    using namespace testing;

    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(_))
        .Times(1);
    mc::SwapperFactory strategy(mock_buffer_allocator);

    size_t size = 2;
    std::vector<std::shared_ptr<mg::Buffer>> list{};
    auto swapper = strategy.create_swapper_reuse_buffers(
        properties, list, size, mc::SwapperType::framedropping);
}
