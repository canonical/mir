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

#include "src/server/compositor/rw_lock.h"
#include <gtest/gtest.h>
#include <thread>

struct RWLockTest : public testing::Test
{
};

TEST_F(RWLockTest, multi_readers)
{
    int const num_asyncs = 33;

    mir::RWLock lock;
    int locked_value = 45;

    std::vector<std::async> asyncs;

    for(auto i = 0; i < num_asyncs; i++)
    {
        asyncs.push_front(std::async(std::launch::async,
            [&]
            {
                std::unique_lock<ReadLock> lk(lock);
                return locked_value; 
            }));
    }

    for(auto& i : asyncs)
    {
        i.wait(); 
    }

    for(auto& i : asyncs)
    {
        EXPECT_EQ(locked_value, i.get()); 
    }
}
