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

class SwapperSwitcherTest : public ::testing::Test
{
}

TEST_F(SwapperSwitcherTest, client_acquire_basic)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer);
    EXPECT_CALL(*mock_default_swapper, client_release(mock_buffer))
        .Times(1);

    auto buffer = switcher.client_acquire();
    switcher.client_release(buffer); 
}

TEST_F(SwapperSwitcherTest, client_acquire_with_switch)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer);
    EXPECT_CALL(*mock_secondary_swapper, client_release(mock_buffer))
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
        .WillOnce(Return(mock_buffer);
    EXPECT_CALL(*mock_default_swapper, compositor_release(mock_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();
    switcher.compositor_release(buffer); 
}

TEST_F(SwapperSwitcherTest, compositor_acquire_with_switch)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(mock_buffer);
    EXPECT_CALL(*mock_secondary_swapper, compositor_release(mock_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();

    switcher.switch_swapper(mock_secondary_swapper);
 
    switcher.compositor_release(buffer); 
}

TEST_F(SwapperSwitcherTest, nonblocking)
{
}
