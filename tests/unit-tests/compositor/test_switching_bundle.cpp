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
#include "mir/compositor/swapper_factory.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_swapper.h"
#include "mir_test_doubles/mock_swapper_factory.h"

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
        mock_swapper_factory = std::make_shared<testing::NiceMock<mtd::MockSwapperFactory>>();
        mock_default_swapper = std::make_shared<testing::NiceMock<mtd::MockSwapper>>();
        mock_secondary_swapper = std::make_shared<testing::NiceMock<mtd::MockSwapper>>();
        stub_buffer = std::make_shared<mtd::StubBuffer>();
        properties = mg::BufferProperties{geom::Size{4, 2},
                                          geom::PixelFormat::abgr_8888, mg::BufferUsage::hardware};

        ON_CALL(*mock_swapper_factory, create_swapper_new_buffers(_,_,_))
            .WillByDefault(Return(mock_default_swapper));
    }

    std::shared_ptr<mtd::MockSwapperFactory> mock_swapper_factory;
    std::shared_ptr<mtd::MockSwapper> mock_default_swapper;
    std::shared_ptr<mtd::MockSwapper> mock_secondary_swapper;
    std::shared_ptr<mg::Buffer> stub_buffer;
    mg::BufferProperties properties;
};

TEST_F(SwitchingBundleTest, sync_swapper_by_default)
{
    using namespace testing;
    auto actual_properties = mg::BufferProperties{geom::Size{7, 8},
                                                  geom::PixelFormat::argb_8888, mg::BufferUsage::software};
    EXPECT_CALL(*mock_swapper_factory, create_swapper_new_buffers(_,_,mc::SwapperType::synchronous))
        .Times(1)
        .WillOnce(DoAll(SetArgReferee<0>(actual_properties),
                        Return(mock_default_swapper)));

    mc::SwitchingBundle switcher(mock_swapper_factory, properties);
    EXPECT_EQ(actual_properties, switcher.properties());
}

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
