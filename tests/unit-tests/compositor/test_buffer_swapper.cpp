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
#include "mir_test_doubles/mock_buffer.h"
#include "mir_test_doubles/mock_id_generator.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>

namespace mc = mir::compositor;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

struct BufferSwapperConstruction : testing::Test
{
    BufferSwapperConstruction()
    {
        geom::Size size {geom::Width{1024}, geom::Height{768}};
        geom::Height h {768};
        geom::Stride s {1024};
        geom::PixelFormat pf {geom::PixelFormat::rgba_8888};

        mock_generator = std::make_shared<mtd::MockIDGenerator>();
        buffer_a = std::make_shared<mtd::MockBuffer>(size, s, pf);
        buffer_b = std::make_shared<mtd::MockBuffer>(size, s, pf);
        buffer_c = std::make_shared<mtd::MockBuffer>(size, s, pf);

        id1 = mc::BufferID{4};
        id2 = mc::BufferID{6};
        id3 = mc::BufferID{5};
    }

    mc::BufferID id1;
    mc::BufferID id2;
    mc::BufferID id3;
    std::shared_ptr<mc::Buffer> buffer_a;
    std::shared_ptr<mc::Buffer> buffer_b;
    std::shared_ptr<mc::Buffer> buffer_c;
    std::shared_ptr<mtd::MockIDGenerator> mock_generator;
};

TEST_F(BufferSwapperConstruction, basic_double_construction)
{
    using namespace testing;
    EXPECT_CALL(*mock_generator, generate_unique_id())
        .Times(2)
        .WillOnce(Return(id1))
        .WillRepeatedly(Return(id2));

    auto use_count_before_a  = buffer_a.use_count();
    auto use_count_before_b  = buffer_b.use_count();
    mc::BufferSwapperMulti swapper(std::move(mock_generator), {buffer_a, buffer_b});

    EXPECT_EQ(buffer_a.use_count(), use_count_before_a + 1);
    EXPECT_EQ(buffer_b.use_count(), use_count_before_b + 1);

    /* just to keep ref */
    swapper.shutdown(); 
}

TEST_F(BufferSwapperConstruction, basic_triple_construction)
{
    using namespace testing;
    EXPECT_CALL(*mock_generator, generate_unique_id())
        .Times(3)
        .WillOnce(Return(id1))
        .WillOnce(Return(id2))
        .WillRepeatedly(Return(id3));

    auto use_count_before  = buffer_a.use_count();
    mc::BufferSwapperMulti swapper(std::move(mock_generator), {buffer_a, buffer_a, buffer_a});

    EXPECT_EQ(buffer_a.use_count(), use_count_before + 3);

    /* just to keep ref */
    swapper.shutdown(); 
}

TEST_F(BufferSwapperConstruction, error_construction)
{
    auto mock_generator2 = std::make_shared<mtd::MockIDGenerator>();
    /* don't support single buffering with the mc::BufferSwapper interface model */
    EXPECT_THROW({
        mc::BufferSwapperMulti(std::move(mock_generator), {buffer_a});
    }, std::runtime_error);

    /* BufferSwapper multi theoretically is generic enough to do >=4 buffers. However, we only test for 2 or 3,
       so we should throw on 4 or greater until 4 or greater buffers is tested*/
    EXPECT_THROW({
        mc::BufferSwapperMulti(std::move(mock_generator2), {buffer_a, buffer_b, buffer_c, buffer_b});
    }, std::runtime_error);
}

TEST_F(BufferSwapperConstruction, double_assigns_unique_to_each)
{
    using namespace testing;
    EXPECT_CALL(*mock_generator, generate_unique_id())
        .Times(2)
        .WillOnce(Return(id1))
        .WillRepeatedly(Return(id2));

    mc::BufferSwapperMulti swapper(std::move(mock_generator), {buffer_a, buffer_b});

    mc::BufferID test_id_1, test_id_2;
    std::weak_ptr<mc::Buffer> buffer_ref; 
    swapper.compositor_acquire(buffer_ref, test_id_1);
    swapper.client_acquire(buffer_ref, test_id_2);
    /* swapper is now 'empty' */

    EXPECT_TRUE((test_id_1 == id1) || (test_id_1 == id2));
    EXPECT_TRUE((test_id_2 == id1) || (test_id_2 == id2));
    EXPECT_NE(test_id_1, test_id_2);
}

TEST_F(BufferSwapperConstruction, triple_assigns_unique_to_each)
{
    using namespace testing;
    EXPECT_CALL(*mock_generator, generate_unique_id())
        .Times(3)
        .WillOnce(Return(id1))
        .WillOnce(Return(id2))
        .WillRepeatedly(Return(id3));

    mc::BufferSwapperMulti swapper(std::move(mock_generator), {buffer_a, buffer_b, buffer_c});

    mc::BufferID test_id_1, test_id_2, test_id_3;
    std::weak_ptr<mc::Buffer> buffer_ref; 
    swapper.compositor_acquire(buffer_ref, test_id_1);
    swapper.client_acquire(buffer_ref, test_id_2);
    swapper.client_release(test_id_2);
    swapper.client_acquire(buffer_ref, test_id_3);
    /* swapper is now 'empty' */

    EXPECT_TRUE((test_id_1 == id1) || (test_id_1 == id2) || (test_id_1 == id3));
    EXPECT_TRUE((test_id_2 == id1) || (test_id_2 == id2) || (test_id_2 == id3));
    EXPECT_TRUE((test_id_3 == id1) || (test_id_3 == id2) || (test_id_3 == id3));
    EXPECT_NE(test_id_1, test_id_2);
    EXPECT_NE(test_id_1, test_id_3);
    EXPECT_NE(test_id_2, test_id_3);
}
