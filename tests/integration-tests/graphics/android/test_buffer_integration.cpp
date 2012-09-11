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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/graphics/android/android_buffer_allocator.h"
#include "mir/compositor/double_buffer_allocation_strategy.h"

#include <gtest/gtest.h>

namespace mc=mir::compositor;
namespace geom=mir::geometry; 
namespace mga=mir::graphics::android; 

class AndroidBufferIntegration : public ::testing::Test
{
public:
    virtual void SetUp()
    {

    }
};


TEST_F(AndroidBufferIntegration, alloc_does_not_throw)
{
    auto width = geom::Width(300);
    auto height = geom::Height(200);
 
    auto allocator = std::make_shared<mga::AndroidBufferAllocator>();
    auto strategy = std::make_shared<mc::DoubleBufferAllocationStrategy>(allocator);
}
