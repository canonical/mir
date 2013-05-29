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

#include "src/server/compositor/swapper_switcher.h"
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
    std::shared_ptr<mc::Buffer> stub_buffer;
};

TEST_F(SwapperSwitcherTest, client_acquire_basic)
{
    using namespace testing;
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_default_swapper, client_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.client_acquire();
    switcher.client_release(buffer); 
}

TEST_F(SwapperSwitcherTest, client_acquire_with_switch)
{
    using namespace testing;
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, client_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_secondary_swapper, client_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.client_acquire();

    switcher.transfer_buffers_to(mock_secondary_swapper);
 
    switcher.client_release(buffer); 
}

TEST_F(SwapperSwitcherTest, compositor_acquire_basic)
{
    using namespace testing;
    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_default_swapper, compositor_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();
    switcher.compositor_release(buffer); 
}

TEST_F(SwapperSwitcherTest, compositor_acquire_with_switch)
{
    using namespace testing;

    mc::SwapperSwitcher switcher(mock_default_swapper);

    EXPECT_CALL(*mock_default_swapper, compositor_acquire())
        .Times(1)
        .WillOnce(Return(stub_buffer));
    EXPECT_CALL(*mock_secondary_swapper, compositor_release(stub_buffer))
        .Times(1);

    auto buffer = switcher.compositor_acquire();

    switcher.transfer_buffers_to(mock_secondary_swapper);
 
    switcher.compositor_release(buffer); 
}

TEST_F(SwapperSwitcherTest, switch_sequence)
{
    using namespace testing;
    mc::SwapperSwitcher switcher(mock_default_swapper);

    InSequence seq;
    EXPECT_CALL(*mock_default_swapper, force_requests_to_complete())
        .Times(1);
    std::shared_ptr<mc::BufferSwapper> tmp = mock_secondary_swapper;
    EXPECT_CALL(*mock_default_swapper, transfer_buffers_to(tmp))
        .Times(1);

    switcher.transfer_buffers_to(mock_secondary_swapper);
}

#if 0
TEST_F(SwapperSwitcherTest, switch_makes_client_threads_wait_and_wakeup)
{
    mc::SwapperSwitcher switcher(mock_default_swapper);

}
#endif

#if 0
#endif
