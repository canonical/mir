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
#include <vector>
#include <future>
#include <thread>

namespace mc=mir::compositor;
TEST(RWLockWriterBiasTest, multi_readers_dont_block)
{
    int const num_asyncs = 33;

    mc::RWLockWriterBias lock;
    int locked_value = 45;

    std::vector<std::future<int>> asyncs;

    {
        mc::ReadLock rd_lk(lock);
        std::unique_lock<mc::ReadLock> lk(rd_lk);
        for(auto i = 0; i < num_asyncs; i++)
        {
            asyncs.push_back(std::async(std::launch::async,
                [&]
                {
                    std::unique_lock<mc::ReadLock> lk(lock);
                    return locked_value; 
                }));
        }

        for(auto& i : asyncs)
        {
            i.wait(); 
        }
    }

    for(auto& i : asyncs)
    {
        EXPECT_EQ(locked_value, i.get()); 
    }
}

TEST(RWLockWriterBiasTest, multi_writers)
{
    int const num_asyncs = 10, num_loops = 100;

    mc::RWLockWriterBias lock;
    int locked_value = 0;

    std::vector<std::future<void>> asyncs;
    for(auto i = 0; i < num_asyncs; i++)
    {
        asyncs.push_back(std::async(std::launch::async,
            [&]
            {
                std::unique_lock<mc::WriteLock> lk(lock);
                for(auto j=0; j < num_loops; j++)
                {
                    locked_value++;
                }
            }));
    }

    for(auto& i : asyncs)
    {
        i.wait(); 
    }

    EXPECT_EQ(num_asyncs * num_loops, locked_value); 
}

TEST(RWLockWriterBiasTest, writers_force_wait)
{
    int const num_asyncs = 100;
    std::vector<std::future<void>> reader_asyncs;

    mc::RWLockWriterBias lock;

    {
        std::unique_lock<mc::WriteLock> lk(lock);
        for(auto i = 0; i < num_asyncs; i++)
        {
            reader_asyncs.push_back(std::async(std::launch::async,
                [&]
                {
                    std::unique_lock<mc::ReadLock> lk(lock);
                }));
        }
    }

    for(auto& i : reader_asyncs)
    {
        i.wait();
    }
}

TEST(RWLockWriterBiasTest, readers_and_writers)
{
    int const num_asyncs = 10;
    std::vector<std::future<int>> reader_asyncs;

    mc::RWLockWriterBias lock;
    int const before = 66, after = 1066;
    int locked_value = before;

    for(auto i = 0; i < num_asyncs; i++)
    {
        reader_asyncs.push_back(std::async(std::launch::async,
            [&]
            {
                std::unique_lock<mc::ReadLock> lk(lock);
                return locked_value; 
            }));
    }

    auto writer_async = std::async(std::launch::async,
        [&]
        {
            std::unique_lock<mc::WriteLock> lk(lock);
            for(auto j=0; j<1000; j++)
            {
                locked_value++;
            }
        });
    writer_async.wait();

    for(auto i = 0; i < num_asyncs; i++)
    {
        reader_asyncs.push_back(std::async(std::launch::async,
            [&]
            {
                std::unique_lock<mc::ReadLock> lk(lock);
                return locked_value; 
            }));
    }

    for(auto& i : reader_asyncs)
    {
        i.wait();
        auto value = i.get();
        EXPECT_TRUE( ((value == before) || (value == after)) ); 
    }
}
