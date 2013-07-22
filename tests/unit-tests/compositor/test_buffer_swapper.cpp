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

#include "mir/compositor/buffer_swapper_multi.h"
#include "mir_test_doubles/stub_buffer.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

struct BufferSwapperConstruction : testing::Test
{
    BufferSwapperConstruction()
    {
        buffer_a = std::make_shared<mtd::StubBuffer>();
        buffer_b = std::make_shared<mtd::StubBuffer>();
        buffer_c = std::make_shared<mtd::StubBuffer>();
    }

    std::shared_ptr<mg::Buffer> buffer_a;
    std::shared_ptr<mg::Buffer> buffer_b;
    std::shared_ptr<mg::Buffer> buffer_c;
};

TEST_F(BufferSwapperConstruction, basic_double_construction_vector)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b, buffer_c};

    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();
    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);

    /* just to keep ref */
    swapper.force_client_abort();
}

TEST_F(BufferSwapperConstruction, basic_double_construction_initializer)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b};

    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();

    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);

    /* just to keep ref */
    swapper.force_client_abort();
}

TEST_F(BufferSwapperConstruction, basic_triple_construction_initializer)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b, buffer_c};
    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();
    auto use_count_before_c  = buffer_c.use_count();
    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);
    EXPECT_EQ(buffer_c.use_count(), use_count_before_c + 1);

    /* just to keep ref */
    swapper.force_client_abort();
}

TEST_F(BufferSwapperConstruction, buffers_out_come_from_init_double)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b};
    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    auto buffer_1 = swapper.compositor_acquire();
    auto buffer_2 = swapper.client_acquire();
    /* swapper is now 'empty' */

    EXPECT_TRUE((buffer_a == buffer_1) || (buffer_b == buffer_1) );
    EXPECT_TRUE((buffer_a == buffer_2) || (buffer_b == buffer_2) );
}

TEST_F(BufferSwapperConstruction, buffers_out_come_from_init_triple)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b, buffer_c};
    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    auto buffer_1 = swapper.compositor_acquire();
    auto buffer_2 = swapper.client_acquire();
    swapper.client_release(buffer_2);
    auto buffer_3 = swapper.client_acquire();
    /* swapper is now 'empty' */

    EXPECT_TRUE((buffer_a == buffer_1) || (buffer_b == buffer_1) || (buffer_c == buffer_1));
    EXPECT_TRUE((buffer_a == buffer_2) || (buffer_b == buffer_2) || (buffer_c == buffer_2));
    EXPECT_TRUE((buffer_a == buffer_3) || (buffer_b == buffer_3) || (buffer_c == buffer_3));
}


TEST_F(BufferSwapperConstruction, buffer_transfer_triple_all_owned)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b, buffer_c};
    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    size_t test_size;
    std::vector<std::shared_ptr<mg::Buffer>> list;
    swapper.end_responsibility(list, test_size);

    auto res1 = std::find(list.begin(), list.end(), buffer_a);
    auto res2 = std::find(list.begin(), list.end(), buffer_b);
    auto res3 = std::find(list.begin(), list.end(), buffer_c);
    EXPECT_EQ(3u, list.size());
    EXPECT_NE(list.end(), res1);
    EXPECT_NE(list.end(), res2);
    EXPECT_NE(list.end(), res3);

    EXPECT_EQ(3u, test_size);
}

TEST_F(BufferSwapperConstruction, buffer_transfer_triple_some_not_owned)
{
    std::vector<std::shared_ptr<mg::Buffer>> buffers{buffer_a, buffer_b, buffer_c};
    mc::BufferSwapperMulti swapper(buffers, buffers.size());

    auto acquired_buffer = swapper.client_acquire();

    size_t test_size;
    std::vector<std::shared_ptr<mg::Buffer>> list;
    swapper.end_responsibility(list, test_size);

    auto res1 = std::find(list.begin(), list.end(), acquired_buffer);
    EXPECT_EQ(2u, list.size());
    EXPECT_EQ(list.end(), res1); //acquired_buffer should not be in list

    EXPECT_EQ(3u, test_size);
}
