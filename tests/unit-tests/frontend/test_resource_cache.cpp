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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "src/server/frontend/resource_cache.h"

#include <thread>
#include <atomic>
#include <gtest/gtest.h>
#include <gmock/gmock.h>

namespace mir
{
namespace
{
struct TestResource
{
    TestResource()
    {
        int count = instances.load();
        while (!instances.compare_exchange_weak(count, count + 1))
            std::this_thread::yield();
    }
    ~TestResource()
    {
        int count = instances.load();
        while (!instances.compare_exchange_weak(count, count - 1))
            std::this_thread::yield();
    }

    TestResource(TestResource const&) = delete;
    TestResource& operator=(TestResource const&) = delete;

    static std::atomic<int> instances;
};

std::atomic<int> TestResource::instances;
}
}
using mir::TestResource;

namespace
{
struct ResourceCache : ::testing::Test
{
    mir::frontend::ResourceCache cache;
};
}

TEST_F(ResourceCache, resources_are_saved)
{
    int const a_few = 13;

    mir::protobuf::Void keys[a_few];

    for (auto p = keys+0; p != keys+a_few; ++p)
    {
        auto sp = std::make_shared<TestResource>();
        cache.save_resource(p, sp);
    }

    EXPECT_EQ(a_few, TestResource::instances.load());
}

TEST_F(ResourceCache, resources_are_freed)
{
    int const a_few = 13;

    mir::protobuf::Void keys[a_few];

    for (auto p = keys+0; p != keys+a_few; ++p)
    {
        auto sp = std::make_shared<TestResource>();
        cache.save_resource(p, sp);
    }

    for (auto p = keys+0; p != keys+a_few; ++p)
    {
        cache.free_resource(p);
    }

    EXPECT_EQ(0, TestResource::instances.load());
}
