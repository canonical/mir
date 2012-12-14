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

class AndroidAllocTest : public ::testing::Test
{
protected:
    virtual void SetUp()
    {
    }
};

TEST_F(AndroidAllocTest, basic)
{
    EXPECT_CALL(mock_id_generator, generate_new_id)
        .Times(2)
        .WillOnce(Return(id1)
        .WillRepeatedly(Return(id2));

    mga::AndroidBufferAllocator allocator(mock_id_generator);
    

    auto buffer1 = allocator.alloc_buffer(properties);
    auto buffer2 = allocator.alloc_buffer(properties);

    EXPECT_EQ(buffer1->id(), id1);
    EXPECT_EQ(buffer2->id(), id2);
}
