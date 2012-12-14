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

#include "src/graphics/android/android_buffer_allocator.h"
#include "mir/compositor/buffer_properties.h"

#include "mir_test_doubles/mock_id_generator.h"

#include <gtest/gtest.h>
#include "mir_test/gmock_fixes.h"

namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mga=mir::graphics::android;
namespace geom=mir::geometry;

class AndroidAllocTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
        id1 = mc::BufferID{4};
        id2 = mc::BufferID{5};
        properties = mc::BufferProperties{geom::Size{geom::Width{44}, geom::Height{75}}, 
                                          geom::PixelFormat::rgba_8888,
                                          mc::BufferUsage::hardware};
        mock_id_generator = std::unique_ptr<mtd::MockIDGenerator>(new mtd::MockIDGenerator);
        /* we move a unique_ptr in test. google mock leak checker doesn't know how to deal with things 
           that are moved, so we manually tell gmock to not check the object */
        testing::Mock::AllowLeak(mock_id_generator.get());
    }

    std::unique_ptr<mtd::MockIDGenerator> mock_id_generator;
    mc::BufferID id1;
    mc::BufferID id2;
    mc::BufferProperties properties;
};

TEST_F(AndroidAllocTest, basic)
{
    using namespace testing;
    EXPECT_CALL(*mock_id_generator, generate_unique_id())
        .Times(2)
        .WillOnce(Return(id1))
        .WillRepeatedly(Return(id2));

    mga::AndroidBufferAllocator allocator(std::move(mock_id_generator));

    auto buffer1 = allocator.alloc_buffer(properties);
    auto buffer2 = allocator.alloc_buffer(properties);

    EXPECT_EQ(buffer1->id(), id1);
    EXPECT_EQ(buffer2->id(), id2);

}
