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

#include "mir/compositor/double_buffer_allocation_strategy.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"
#include "mir_test/mock_buffer.h"
#include "mir_test/mock_graphic_buffer_allocator.h"


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

#if defined(MIR_DEATH_TESTS_ENABLED)
TEST(
    double_buffer_allocation_strategy_death_test,
    if_dependency_on_allocator_is_missing_an_assertion_is_triggered)
{
// Trying to avoid "[WARNING] /usr/src/gtest/src/gtest-death-test.cc:789::
// Death tests use fork(), which is unsafe particularly in a threaded context.
// For this test, Google Test couldn't detect the number of threads." by
//  ::testing::FLAGS_gtest_death_test_style = "threadsafe";
// leads to the test failing under valgrind
    ::testing::FLAGS_gtest_death_test_style = "fast";
    EXPECT_EXIT(mc::DoubleBufferAllocationStrategy(std::shared_ptr<mc::GraphicBufferAllocator>()), ::testing::KilledBySignal(SIGABRT), ".*");
}
#endif // defined(MIR_DEATH_TESTS_ENABLED)

