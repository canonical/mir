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
 * Authored by: Thomas Voss <thomas.voss@canonical.com>
 */

#include "mir_test/mock_buffer.h"

#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_swapper.h"
#include "mir/compositor/buffer_allocation_strategy.h"
#include "mir/compositor/buffer_bundle_manager.h"
#include "mir/compositor/buffer_bundle.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/compositor/buffer_properties.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir_test/empty_deleter.h"

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{


struct MockBufferAllocationStrategy : public mc::BufferAllocationStrategy
{
    MOCK_METHOD1(
        create_swapper,
        std::unique_ptr<mc::BufferSwapper>(mc::BufferProperties const&));
};

geom::Size size{geom::Width{1024}, geom::Height{768}};
const geom::Stride stride{geom::dim_cast<geom::Stride>(size.width)};
const geom::PixelFormat pixel_format{geom::PixelFormat::rgba_8888};
const mc::BufferProperties buffer_properties{size, pixel_format};

}

TEST(buffer_manager, create_buffer)
{
    using namespace testing;

    mc::MockBuffer mock_buffer{size, stride, pixel_format};
    std::shared_ptr<mc::MockBuffer> default_buffer(
        &mock_buffer,
        mir::EmptyDeleter());
    MockBufferAllocationStrategy allocation_strategy;

    mc::BufferBundleManager buffer_bundle_manager(
            std::shared_ptr<mc::BufferAllocationStrategy>(&allocation_strategy, mir::EmptyDeleter()));


    EXPECT_CALL(allocation_strategy, create_swapper(buffer_properties))
        .Times(AtLeast(1));

    std::shared_ptr<mc::BufferBundle> bundle{
        buffer_bundle_manager.create_buffer_bundle(buffer_properties)};

    EXPECT_TRUE(bundle.get());
}
