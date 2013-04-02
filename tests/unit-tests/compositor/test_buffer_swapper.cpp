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
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

struct BufferSwapperConstruction : testing::Test
{
    BufferSwapperConstruction()
    {
        buffer_a = std::make_shared<mtd::StubBuffer>();
        buffer_b = std::make_shared<mtd::StubBuffer>();
        buffer_c = std::make_shared<mtd::StubBuffer>();

        id1 = buffer_a->id();
        id2 = buffer_b->id();
        id3 = buffer_c->id();
    }

    bool check_ref(std::shared_ptr<mc::Buffer> buffer, mc::BufferID id)
    {
        if ((id == id1) && (buffer.get() == buffer_a.get()))
            return true;
        if ((id == id2) && (buffer.get() == buffer_b.get()))
            return true;
        if ((id == id3) && (buffer.get() == buffer_c.get()))
            return true;

        return false;
    }

    mc::BufferID id1;
    mc::BufferID id2;
    mc::BufferID id3;
    std::shared_ptr<mc::Buffer> buffer_a;
    std::shared_ptr<mc::Buffer> buffer_b;
    std::shared_ptr<mc::Buffer> buffer_c;
};

TEST_F(BufferSwapperConstruction, basic_double_construction_vector)
{
    std::vector<std::shared_ptr<mc::Buffer>> buffers{buffer_a, buffer_b, buffer_c};

    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();
    mc::BufferSwapperMulti swapper(buffers);

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);

    /* just to keep ref */
    swapper.shutdown();
}

TEST_F(BufferSwapperConstruction, basic_double_construction_initializer)
{
    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();
    mc::BufferSwapperMulti swapper({buffer_a, buffer_b});

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);

    /* just to keep ref */
    swapper.shutdown();
}

TEST_F(BufferSwapperConstruction, basic_triple_construction_initializer)
{
    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();
    auto use_count_before_c  = buffer_c.use_count();
    mc::BufferSwapperMulti swapper({buffer_a, buffer_b, buffer_c});

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);
    EXPECT_EQ(buffer_c.use_count(), use_count_before_c + 1);

    /* just to keep ref */
    swapper.shutdown();
}

TEST_F(BufferSwapperConstruction, error_construction)
{
    /* don't support single buffering with the mc::BufferSwapper interface model */
    EXPECT_THROW({
        mc::BufferSwapperMulti({buffer_a});
    }, std::logic_error);

    /* BufferSwapperMulti algorithm is generic enough to do >=4 buffers. However, we have only tested
       for 2 or 3 buffers, so we will throw on 4 or greater until 4 or greater buffers is tested*/
    EXPECT_THROW({
        mc::BufferSwapperMulti({buffer_a, buffer_b, buffer_c, buffer_b});
    }, std::logic_error);
}

TEST_F(BufferSwapperConstruction, references_match_ids_double)
{
    mc::BufferSwapperMulti swapper({buffer_a, buffer_b});

    mc::BufferID test_id_1, test_id_2;
    std::shared_ptr<mc::Buffer> buffer_ref_1, buffer_ref_2;
    swapper.compositor_acquire(buffer_ref_1, test_id_1);
    swapper.client_acquire(buffer_ref_2, test_id_2);
    /* swapper is now 'empty' */

    /* test uniqueness of ids */
    EXPECT_TRUE((test_id_1 == id1) || (test_id_1 == id2));
    EXPECT_TRUE((test_id_2 == id1) || (test_id_2 == id2));
    EXPECT_NE(test_id_1, test_id_2);

    /* test id's match refs */
    EXPECT_TRUE(check_ref(buffer_ref_1, test_id_1));
    EXPECT_TRUE(check_ref(buffer_ref_2, test_id_2));
}

TEST_F(BufferSwapperConstruction, references_match_ids_triple)
{
    mc::BufferSwapperMulti swapper({buffer_a, buffer_b, buffer_c});

    mc::BufferID test_id_1, test_id_2, test_id_3;
    std::shared_ptr<mc::Buffer> buffer_ref_1, buffer_ref_2, buffer_ref_3;
    swapper.compositor_acquire(buffer_ref_1, test_id_1);
    swapper.client_acquire(buffer_ref_2, test_id_2);
    swapper.client_release(test_id_2);
    swapper.client_acquire(buffer_ref_3, test_id_3);
    /* swapper is now 'empty' */

    /* test uniqueness of ids */
    EXPECT_TRUE((test_id_1 == id1) || (test_id_1 == id2) || (test_id_1 == id3));
    EXPECT_TRUE((test_id_2 == id1) || (test_id_2 == id2) || (test_id_2 == id3));
    EXPECT_TRUE((test_id_3 == id1) || (test_id_3 == id2) || (test_id_3 == id3));
    EXPECT_NE(test_id_1, test_id_2);
    EXPECT_NE(test_id_1, test_id_3);
    EXPECT_NE(test_id_2, test_id_3);

    /* test id's match refs */
    EXPECT_TRUE(check_ref(buffer_ref_1, test_id_1));
    EXPECT_TRUE(check_ref(buffer_ref_2, test_id_2));
    EXPECT_TRUE(check_ref(buffer_ref_3, test_id_3));
}
