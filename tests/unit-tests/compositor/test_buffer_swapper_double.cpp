/*
 * Copyright © 2012, 2013 Canonical Ltd.
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

#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/compositor/buffer_id.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{
struct BufferSwapperDouble : testing::Test
{
    BufferSwapperDouble()
    {
        buffer_a = std::make_shared<mtd::StubBuffer>();
        buffer_b = std::make_shared<mtd::StubBuffer>();

        auto double_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b};
        swapper = std::make_shared<mc::BufferSwapperMulti>(double_list);

    }

    std::shared_ptr<mc::Buffer> buffer_a;
    std::shared_ptr<mc::Buffer> buffer_b;

    std::shared_ptr<mc::BufferSwapper> swapper;
};

}

TEST_F(BufferSwapperDouble, test_valid_buffer_returned)
{
    auto buffer = swapper->client_acquire();

    EXPECT_TRUE((buffer == buffer_a) || (buffer == buffer_b));
}

TEST_F(BufferSwapperDouble, test_valid_and_unique_with_two_acquires)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    auto buffer_tmp = swapper->compositor_acquire();
    swapper->compositor_release(buffer_tmp);

    auto buffer_2 = swapper->client_acquire();
    swapper->client_release(buffer_2);

    EXPECT_NE(buffer_1, buffer_2);

    EXPECT_TRUE((buffer_1 == buffer_a) || (buffer_1 == buffer_b));
    EXPECT_TRUE((buffer_2 == buffer_a) || (buffer_2 == buffer_b));
}

TEST_F(BufferSwapperDouble, test_compositor_gets_valid)
{
    auto buffer = swapper->compositor_acquire();
    EXPECT_TRUE((buffer == buffer_a) || (buffer == buffer_b));
}

TEST_F(BufferSwapperDouble, test_compositor_gets_last_posted)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    auto buffer_2 = swapper->compositor_acquire();
    swapper->compositor_release(buffer_2);

    EXPECT_EQ(buffer_1, buffer_2);
}

TEST_F(BufferSwapperDouble, test_two_grabs_without_a_client_release)
{
    swapper->client_acquire();

    auto buffer_1 = swapper->compositor_acquire();
    swapper->compositor_release(buffer_1);

    auto buffer_2 = swapper->compositor_acquire();
    EXPECT_EQ(buffer_1, buffer_2);
}

TEST_F(BufferSwapperDouble, test_client_and_compositor_advance_buffers_in_serial_pattern)
{
    auto buffer_1 = swapper->client_acquire();
    swapper->client_release(buffer_1);

    auto buffer_2 = swapper->compositor_acquire();
    swapper->compositor_release(buffer_2);

    auto buffer_3 = swapper->client_acquire();
    swapper->client_release(buffer_3);

    auto buffer_4 = swapper->compositor_acquire();

    EXPECT_NE(buffer_1, buffer_3);
    EXPECT_NE(buffer_2, buffer_4);
}
