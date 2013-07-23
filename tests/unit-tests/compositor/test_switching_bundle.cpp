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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "src/server/compositor/switching_bundle.h"
#include "mir_test_doubles/mock_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mg = mir::graphics;

struct SwitchingBundleTest : public ::testing::Test
{
    void SetUp()
    {
        using namespace testing;
        mock_buffer_allocator = std::make_shared<mtd::MockBufferAllocator>();
    }

    std::shared_ptr<mtd::MockBufferAllocator> mock_buffer_allocator;
};

TEST_F(SwitchingBundleTest, TODO_sync_swapper_by_default)
{
    using namespace testing;
    mc::BufferProperties properties{geom::Size{7, 8},
                                    geom::PixelFormat::argb_8888,
                                    mc::BufferUsage::software};
    EXPECT_CALL(*mock_buffer_allocator, alloc_buffer(_))
        .Times(2);

    mc::SwitchingBundle switcher(2, mock_buffer_allocator, properties);
    EXPECT_EQ(properties, switcher.properties());
}

#if 0 // TODO
TEST_F(SwitchingBundleTest, client_acquire_basic)
{
    using namespace testing;
    mc::SwitchingBundle switcher(mock_swapper_factory, properties);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_default_swapper, client_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.client_acquire();
    switcher.client_release(buffer); 
}

TEST_F(SwitchingBundleTest, client_acquire_with_switch)
{
    using namespace testing;
    mc::SwitchingBundle switcher(mock_swapper_factory, properties);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_secondary_swapper, client_release(stub_buffer))
        .Times(1);
    EXPECT_CALL(*mock_swapper_factory, create_swapper_reuse_buffers(_,_,_,_))
        .Times(1)
        .WillOnce(Return(mock_secondary_swapper));

    auto buffer = switcher.client_acquire();
    switcher.allow_framedropping(true);
 
    switcher.client_release(buffer); 
}

TEST_F(SwitchingBundleTest, compositor_acquire_basic)
{
    using namespace testing;
    mc::SwitchingBundle switcher(mock_swapper_factory, properties);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_default_swapper, compositor_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();
    switcher.compositor_release(buffer); 
}

TEST_F(SwitchingBundleTest, compositor_acquire_with_switch)
{
    using namespace testing;

    mc::SwitchingBundle switcher(mock_swapper_factory, properties);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_secondary_swapper, compositor_release(stub_buffer))
        .Times(1);
    EXPECT_CALL(*mock_swapper_factory, create_swapper_reuse_buffers(_,_,_,_))
        .Times(1)
        .WillOnce(Return(mock_secondary_swapper));

    auto buffer = switcher.compositor_acquire();

    switcher.allow_framedropping(true);
 
    switcher.compositor_release(buffer); 
}

TEST_F(SwitchingBundleTest, switch_sequence)
{
    using namespace testing;
    mc::SwitchingBundle switcher(mock_swapper_factory, properties);

    InSequence seq;
    EXPECT_CALL(*mock_default_swapper, force_client_abort())
        .Times(1);
    EXPECT_CALL(*mock_default_swapper, end_responsibility(_,_))
        .Times(1);
    EXPECT_CALL(*mock_swapper_factory, create_swapper_reuse_buffers(_,_,_,mc::SwapperType::framedropping))
        .Times(1)
        .WillOnce(Return(mock_secondary_swapper));

    switcher.allow_framedropping(true);
}
#endif
