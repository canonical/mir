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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */


#include <std/SortedVector.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{
struct AndroidInputSortedVector : public ::testing::Test
{
    android::SortedVector<int> test_vector;

    void SetUp()
    {
        test_vector.clear();
    }
};
}

TEST_F(AndroidInputSortedVector, empty_vector_isEmpty)
{
    EXPECT_TRUE(test_vector.isEmpty());
}

TEST_F(AndroidInputSortedVector, non_empty_vector_not_isEmpty)
{
    test_vector.add(1);
    EXPECT_FALSE(test_vector.isEmpty());
}

TEST_F(AndroidInputSortedVector, items_found_at_expected_indexes)
{
    test_vector.add(3);
    test_vector.add(1);
    test_vector.add(2);
    test_vector.add(0);

    EXPECT_EQ(0, test_vector.indexOf(0));
    EXPECT_EQ(1, test_vector.indexOf(1));
    EXPECT_EQ(2, test_vector.indexOf(2));
    EXPECT_EQ(3, test_vector.indexOf(3));

    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(4));
}

TEST_F(AndroidInputSortedVector, duplicate_adds_create_single_entry)
{
    test_vector.add(3);
    test_vector.add(1);
    test_vector.add(2);
    test_vector.add(0);

    test_vector.add(3);
    test_vector.add(1);
    test_vector.add(2);
    test_vector.add(0);

    EXPECT_EQ(0, test_vector.indexOf(0));
    EXPECT_EQ(1, test_vector.indexOf(1));
    EXPECT_EQ(2, test_vector.indexOf(2));
    EXPECT_EQ(3, test_vector.indexOf(3));

    EXPECT_EQ(4u, test_vector.size());

    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(4));
}

TEST_F(AndroidInputSortedVector, adds_returns_correct_index)
{
    EXPECT_EQ(0, test_vector.add(3));
    EXPECT_EQ(0, test_vector.add(1));
    EXPECT_EQ(1, test_vector.add(2));
    EXPECT_EQ(0, test_vector.add(0));

    EXPECT_EQ(3, test_vector.add(3));
    EXPECT_EQ(1, test_vector.add(1));
    EXPECT_EQ(2, test_vector.add(2));
    EXPECT_EQ(0, test_vector.add(0));
}

TEST_F(AndroidInputSortedVector, missing_items_are_not_found)
{
    test_vector.add(3);
    test_vector.add(1);
    test_vector.add(2);
    test_vector.add(0);

    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(4));
}

TEST_F(AndroidInputSortedVector, index_gets_expected_items)
{
    test_vector.add(30);
    test_vector.add(10);
    test_vector.add(20);
    test_vector.add(00);

    EXPECT_EQ(00, test_vector.itemAt(0));
    EXPECT_EQ(10, test_vector.itemAt(1));
    EXPECT_EQ(20, test_vector.itemAt(2));
    EXPECT_EQ(30, test_vector.itemAt(3));
}

TEST_F(AndroidInputSortedVector, edit_changes_item)
{
    test_vector.add(30);
    test_vector.add(10);
    test_vector.add(20);
    test_vector.add(00);
    ASSERT_EQ(10, test_vector.itemAt(1));

    test_vector.editItemAt(1) = 12;

    EXPECT_EQ(12, test_vector.itemAt(1));
}

TEST_F(AndroidInputSortedVector, removed_items_are_not_found)
{
    test_vector.add(3);
    test_vector.add(1);
    test_vector.add(2);
    test_vector.add(0);

    auto const old_index = test_vector.remove(1);

    EXPECT_EQ(1, old_index);
    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(1));
}

TEST_F(AndroidInputSortedVector, remove_unknown_items_is_noop)
{
    test_vector.add(3);
    test_vector.add(2);
    test_vector.add(0);

    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.remove(1));
    EXPECT_EQ(3u, test_vector.size());
}

TEST_F(AndroidInputSortedVector, item_at_removed_index_are_not_found)
{
    test_vector.add(30);
    test_vector.add(10);
    test_vector.add(20);
    test_vector.add(00);
    ASSERT_EQ(10, test_vector.itemAt(1));

    auto const old_index = test_vector.removeItemsAt(1);

    EXPECT_EQ(1, old_index);
    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(10));
}

TEST_F(AndroidInputSortedVector, count_items_at_removed_index_are_not_found)
{
    test_vector.add(30);
    test_vector.add(10);
    test_vector.add(20);
    test_vector.add(00);
    ASSERT_EQ(10, test_vector.itemAt(1));

    auto const old_index = test_vector.removeItemsAt(1, 2);

    EXPECT_EQ(1, old_index);
    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(10));
    EXPECT_EQ(android::NAME_NOT_FOUND, test_vector.indexOf(20));
    EXPECT_NE(android::NAME_NOT_FOUND, test_vector.indexOf(00));
    EXPECT_NE(android::NAME_NOT_FOUND, test_vector.indexOf(30));
}

// Android utils use ALOG_ASSERT - which tends to bomb the program
TEST_F(AndroidInputSortedVector, remove_beyond_end_fails)
{
    test_vector.add(30);
    test_vector.add(10);
    test_vector.add(20);
    test_vector.add(00);

    EXPECT_EQ(android::BAD_VALUE, test_vector.removeItemsAt(4, 1));
    EXPECT_EQ(android::BAD_VALUE, test_vector.removeItemsAt(3, 2));
    EXPECT_EQ(android::BAD_VALUE, test_vector.removeItemsAt(2, 3));
    EXPECT_EQ(android::BAD_VALUE, test_vector.removeItemsAt(1, 4));
    EXPECT_EQ(android::BAD_VALUE, test_vector.removeItemsAt(0, 5));

    EXPECT_EQ(4u, test_vector.size());
}
