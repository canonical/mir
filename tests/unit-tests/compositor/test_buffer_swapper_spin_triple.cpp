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
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#include "mir/compositor/buffer_swapper_spin.h"

#include "mir_test_doubles/stub_buffer.h"

#include <gtest/gtest.h>

#include <algorithm>

namespace mc = mir::compositor;
namespace mtd = mir::test::doubles;

namespace
{

struct BufferSwapperSpinTriple : testing::Test
{
    BufferSwapperSpinTriple()
        : buffer_a{std::make_shared<mtd::StubBuffer>()},
          buffer_b{std::make_shared<mtd::StubBuffer>()},
          buffer_c{std::make_shared<mtd::StubBuffer>()},
          swapper{std::make_shared<mc::BufferSwapperSpin>(
                      std::initializer_list<std::shared_ptr<mc::Buffer>>(
                          {buffer_a, buffer_b, buffer_c}))}
    {
    }

    std::shared_ptr<mc::Buffer> const buffer_a;
    std::shared_ptr<mc::Buffer> const buffer_b;
    std::shared_ptr<mc::Buffer> const buffer_c;

    std::shared_ptr<mc::BufferSwapper> const swapper;
};

}

TEST_F(BufferSwapperSpinTriple, client_can_always_get_new_buffer)
{
    std::vector<std::shared_ptr<mc::Buffer>> expected_buffers;

    for (unsigned int i = 0; i < 10; i++)
    {
        auto buf = swapper->client_acquire();

        /* Check that the client doesn't reuse a buffer until there is no choice */
        if (expected_buffers.empty())
        {
            expected_buffers.push_back(buffer_a);
            expected_buffers.push_back(buffer_b);
            expected_buffers.push_back(buffer_c);
        }

        auto buf_iter = std::find(expected_buffers.begin(), expected_buffers.end(), buf);
        bool acquired_buf_in_expected_buffers = (buf_iter != expected_buffers.end());

        EXPECT_TRUE(acquired_buf_in_expected_buffers) << "buf: " << buf;

        if (acquired_buf_in_expected_buffers)
            expected_buffers.erase(buf_iter);

        swapper->client_release(buf);
    }
}

TEST_F(BufferSwapperSpinTriple, client_can_always_get_new_buffer_while_compositor_has_one)
{
    std::vector<std::shared_ptr<mc::Buffer>> expected_buffers;

    auto comp_buf = swapper->compositor_acquire();

    for (unsigned int i = 0; i < 10; i++)
    {
        auto buf = swapper->client_acquire();

        /* Check that the client doesn't reuse a buffer until there is no choice */
        if (expected_buffers.empty())
        {
            if (comp_buf != buffer_a)
                expected_buffers.push_back(buffer_a);
            if (comp_buf != buffer_b)
                expected_buffers.push_back(buffer_b);
            if (comp_buf != buffer_c)
                expected_buffers.push_back(buffer_c);
        }

        auto buf_iter = std::find(expected_buffers.begin(), expected_buffers.end(), buf);
        bool acquired_buf_in_expected_buffers = (buf_iter != expected_buffers.end());

        EXPECT_TRUE(acquired_buf_in_expected_buffers) << "buf: " << buf;

        if (acquired_buf_in_expected_buffers)
            expected_buffers.erase(buf_iter);

        swapper->client_release(buf);
    }

    swapper->compositor_release(comp_buf);
}

TEST_F(BufferSwapperSpinTriple, compositor_gets_last_posted_client_buffer)
{
    auto buf_a = swapper->client_acquire();
    swapper->client_release(buf_a);

    auto comp_buf1 = swapper->compositor_acquire();
    swapper->compositor_release(comp_buf1);

    EXPECT_EQ(buf_a, comp_buf1);

    auto comp_buf2 = swapper->compositor_acquire();
    swapper->compositor_release(comp_buf2);

    EXPECT_EQ(buf_a, comp_buf2);
}

TEST_F(BufferSwapperSpinTriple, compositor_gets_last_posted_client_buffer_interleaved)
{
    auto buf_a = swapper->client_acquire();
    swapper->client_release(buf_a);

    auto comp_buf1 = swapper->compositor_acquire();

    auto buf_b = swapper->client_acquire();
    swapper->client_release(buf_b);

    swapper->compositor_release(comp_buf1);

    EXPECT_EQ(buf_a, comp_buf1);

    auto comp_buf2 = swapper->compositor_acquire();
    swapper->compositor_release(comp_buf2);

    EXPECT_EQ(buf_b, comp_buf2);
}

TEST_F(BufferSwapperSpinTriple, compositor_release_makes_buffer_available_to_client)
{
    auto comp_buf = swapper->compositor_acquire();

    /* Check that the compositor's buffer is not available to the client */
    for (auto i = 0; i < 3; i++)
    {
        auto buf = swapper->client_acquire();
        EXPECT_NE(comp_buf, buf);
        swapper->client_release(buf);
    }

    swapper->compositor_release(comp_buf);

    /* After the release, the compositor's buffer should be available to the client */
    std::vector<std::shared_ptr<mc::Buffer>> client_buffers;

    for (auto i = 0; i < 3; i++)
    {
        auto buf = swapper->client_acquire();
        client_buffers.push_back(buf);
        swapper->client_release(buf);
    }

    EXPECT_TRUE(client_buffers[0] == comp_buf ||
                client_buffers[1] == comp_buf ||
                client_buffers[2] == comp_buf);
}
