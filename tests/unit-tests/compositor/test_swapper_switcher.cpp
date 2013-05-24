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

#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/mock_swapper.h"

#include <gtest/gtest.h>

namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;

struct SwapperSwitcherTest : public ::testing::Test
{
    void SetUp()
    {
        mock_default_swapper = std::make_shared<mtd::MockSwapper>();
        mock_secondary_swapper = std::make_shared<mtd::MockSwapper>();
        stub_buffer = std::make_shared<mtd::StubBuffer>();
    }

    std::shared_ptr<mtd::MockSwapper> mock_default_swapper;
    std::shared_ptr<mtd::MockSwapper> mock_secondary_swapper;
    std::shared_ptr<mtd::StubBuffer> stub_buffer;
};

TEST_F(SwapperSwitcherTest, client_acquire_basic)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer);
    EXPECT_CALL(*mock_default_swapper, client_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.client_acquire();
    switcher.client_release(buffer); 
}

TEST_F(SwapperSwitcherTest, client_acquire_with_switch)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer);
    EXPECT_CALL(*mock_secondary_swapper, client_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.client_acquire();

    switcher.switch_swapper(mock_secondary_swapper);
 
    switcher.client_release(buffer); 
}

TEST_F(SwapperSwitcherTest, compositor_acquire_basic)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer);
    EXPECT_CALL(*mock_default_swapper, compositor_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();
    switcher.compositor_release(buffer); 
}

TEST_F(SwapperSwitcherTest, compositor_acquire_with_switch)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer);
    EXPECT_CALL(*mock_secondary_swapper, compositor_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();

    switcher.switch_swapper(mock_secondary_swapper);
 
    switcher.compositor_release(buffer); 
}

TEST_F(SwapperSwitcherTest, nonblocking)
{
}
