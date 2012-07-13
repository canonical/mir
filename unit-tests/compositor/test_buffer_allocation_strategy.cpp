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

#include "mock_buffer.h"
#include "mock_graphic_buffer_allocator.h"

#include "mir/compositor/fixed_count_buffer_allocation_strategy.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"
#include "mir/compositor/triple_buffer_allocation_strategy.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;

namespace
{
struct EmptyDeleter
{
    template<typename T>
    void operator()(T* ) const
    {
    }    
};
}

TEST(
    fixed_count_buffer_allocation_strategy_death_test,
    if_dependency_on_allocator_is_missing_an_assertion_is_triggered)
{
    ::testing::FLAGS_gtest_death_test_style = "threadsafe";
    EXPECT_EXIT(mc::DoubleBufferAllocationStrategy(std::shared_ptr<mc::GraphicBufferAllocator>()), ::testing::KilledBySignal(SIGABRT), ".*");
}

TEST(
    fixed_count_buffer_allocation_strategy,
    if_count_is_5_graphic_buffer_allocator_is_called_exactly_5_times)
{
    using namespace ::testing;
    
    static const unsigned int buffer_count{5};
    
    geom::Width w{1024};
    geom::Height h{768};
    geom::Stride s{1024};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};    
    std::shared_ptr<mc::Buffer> buffer{new mc::MockBuffer(w, h, s, pf)};
    mc::MockGraphicBufferAllocator gr_alloc;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator(&gr_alloc, EmptyDeleter());
    EXPECT_CALL(gr_alloc, alloc_buffer(w, h, pf))
            .Times(Exactly(buffer_count))
            .WillRepeatedly(Return(buffer));
    
    mc::BufferBundle buffer_bundle;
    mc::FixedCountBufferAllocationStrategy<buffer_count> strategy(allocator);
    strategy.allocate_buffers_for_bundle(w, h, pf, &buffer_bundle);    
}

TEST(
    double_buffer_allocation_strategy,
    allocates_exactly_two_buffers)
{
    using namespace ::testing;
    
    geom::Width w{1024};
    geom::Height h{768};
    geom::Stride s{1024};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};    
    std::shared_ptr<mc::Buffer> buffer{new mc::MockBuffer(w, h, s, pf)};
    mc::MockGraphicBufferAllocator gr_alloc;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator(&gr_alloc, EmptyDeleter());
    EXPECT_CALL(gr_alloc, alloc_buffer(w, h, pf))
            .Times(Exactly(2))
            .WillRepeatedly(Return(buffer));
    
    mc::BufferBundle buffer_bundle;
    mc::DoubleBufferAllocationStrategy strategy(allocator);
    strategy.allocate_buffers_for_bundle(w, h, pf, &buffer_bundle);
}

TEST(
    triple_buffer_allocation_strategy,
    allocates_exactly_three_buffers)
{
    using namespace ::testing;
    
    geom::Width w{1024};
    geom::Height h{768};
    geom::Stride s{1024};
    mc::PixelFormat pf{mc::PixelFormat::rgba_8888};    
    std::shared_ptr<mc::Buffer> buffer{new mc::MockBuffer(w, h, s, pf)};
    mc::MockGraphicBufferAllocator gr_alloc;
    std::shared_ptr<mc::GraphicBufferAllocator> allocator(&gr_alloc, EmptyDeleter());
    EXPECT_CALL(gr_alloc, alloc_buffer(w, h, pf))
            .Times(Exactly(3))
            .WillRepeatedly(Return(buffer));
    
    mc::BufferBundle buffer_bundle;
    mc::TripleBufferAllocationStrategy strategy(allocator);
    strategy.allocate_buffers_for_bundle(w, h, pf, &buffer_bundle);
}
