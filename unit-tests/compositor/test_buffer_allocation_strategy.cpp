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

#include "mir/compositor/fixed_count_buffer_allocation_strategy.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mock_buffer.h"
#include "mock_graphic_buffer_allocator.h"


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
    EXPECT_EXIT(mc::DoubleBufferAllocationStrategy(std::shared_ptr<mc::GraphicBufferAllocator>()), ::testing::KilledBySignal(SIGABRT), ".*");
}


