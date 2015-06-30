/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir/thread_safe_list.h"
#include "mir/test/wait_condition.h"

#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace
{

struct Dummy {};
using Element = std::shared_ptr<Dummy>;
using SharedPtrList = mir::ThreadSafeList<Element>;

struct ThreadSafeListTest : testing::Test
{
    SharedPtrList list;

    Element const element1 = std::make_shared<Dummy>();
    Element const element2 = std::make_shared<Dummy>();
};

}

TEST_F(ThreadSafeListTest, can_remove_element_while_iterating_same_element)
{
    using namespace testing;

    list.add(element1);

    list.for_each(
        [&] (Element const& element)
        {
            list.remove(element);
        });

    int elements_seen = 0;

    list.for_each(
        [&] (Element const&)
        {
            ++elements_seen;
        });

    EXPECT_THAT(elements_seen, Eq(0));
}

TEST_F(ThreadSafeListTest, can_remove_unused_element_while_iterating_different_element)
{
    using namespace testing;

    list.add(element1);
    list.add(element2);

    int elements_seen = 0;

    list.for_each(
        [&] (Element const&)
        {
            list.remove(element2);
            ++elements_seen;
        });

    EXPECT_THAT(elements_seen, Eq(1));
}

TEST_F(ThreadSafeListTest,
       can_remove_unused_element_while_different_element_is_used_in_different_thread)
{
    using namespace testing;

    list.add(element1);
    list.add(element2);

    mir::test::WaitCondition first_element_in_use;
    mir::test::WaitCondition second_element_removed;

    int elements_seen = 0;

    std::thread t{
        [&]
        {
            list.for_each(
                [&] (Element const&)
                {
                    first_element_in_use.wake_up_everyone();
                    second_element_removed.wait_for_at_most_seconds(3);
                    EXPECT_TRUE(second_element_removed.woken());
                    ++elements_seen;
                });
        }};

    first_element_in_use.wait_for_at_most_seconds(3);
    list.remove(element2);
    second_element_removed.wake_up_everyone();

    t.join();

    EXPECT_THAT(elements_seen, Eq(1));
}

TEST_F(ThreadSafeListTest, removes_all_matching_elements)
{
    using namespace testing;

    std::vector<Element> elements_seen;

    list.add(element1);
    list.add(element2);
    list.add(element1);

    list.remove_all(element1);

    list.for_each(
        [&] (Element const& element)
        {
            elements_seen.push_back(element);
        });

    EXPECT_THAT(elements_seen, ElementsAre(element2));
}

TEST_F(ThreadSafeListTest, clears_all_elements)
{
    using namespace testing;

    int elements_seen = 0;

    list.add(element1);
    list.add(element2);
    list.add(element1);

    list.clear();

    list.for_each(
        [&] (Element const&)
        {
            ++elements_seen;
        });

    EXPECT_THAT(elements_seen, Eq(0));
}
