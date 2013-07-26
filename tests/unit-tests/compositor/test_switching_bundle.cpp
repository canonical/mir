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
 * Authored by: Daniel van Vugt <daniel.van.vugt@canonical.com>
 */

#include "src/server/compositor/switching_bundle.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gtest/gtest.h>

#include <thread>

namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mg=mir::graphics;

using namespace testing;

struct SwitchingBundleTest : public ::testing::Test
{
    void SetUp()
    {
        allocator = std::make_shared<mtd::StubBufferAllocator>();
        basic_properties =
        {
            geom::Size{3, 4},
            geom::PixelFormat::abgr_8888,
            mc::BufferUsage::hardware
        };
    }

    std::shared_ptr<mtd::StubBufferAllocator> allocator;

    mc::BufferProperties basic_properties;
};

TEST_F(SwitchingBundleTest, sync_swapper_by_default)
{
    mc::BufferProperties properties{geom::Size{7, 8},
                                    geom::PixelFormat::argb_8888,
                                    mc::BufferUsage::software};

    mc::SwitchingBundle bundle(2, allocator, properties);

    EXPECT_FALSE(bundle.framedropping_allowed());
    EXPECT_EQ(properties, bundle.properties());
}

TEST_F(SwitchingBundleTest, client_acquire_basic)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);

    auto buffer = bundle.client_acquire();
    bundle.client_release(buffer); 
}

namespace
{
    void composite_thread(mc::SwitchingBundle &bundle,
                          mg::BufferID &composited)
    {
        auto buffer = bundle.compositor_acquire();
        composited = buffer->id();
        bundle.compositor_release(buffer);
    }
}

TEST_F(SwitchingBundleTest, is_really_synchronous)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);
    mg::BufferID prev_id, prev_prev_id;

    ASSERT_FALSE(bundle.framedropping_allowed());

    for (int i = 0; i < 100; i++)
    {
        auto client1 = bundle.client_acquire();
        bundle.client_release(client1);

        mg::BufferID expect_id = client1->id(), composited_id;

        std::thread compositor(composite_thread,
                               std::ref(bundle),
                               std::ref(composited_id));

        compositor.join();
        ASSERT_EQ(expect_id, composited_id);
        
        if (i >= 2)
            ASSERT_EQ(composited_id, prev_prev_id);

        if (i >= 1)
            ASSERT_NE(composited_id, prev_id);

        prev_prev_id = prev_id;
        prev_id = composited_id;
    }
}

TEST_F(SwitchingBundleTest, framedropping_clients_never_block)
{
    mc::SwitchingBundle bundle(3, allocator, basic_properties);

    bundle.allow_framedropping(true);
    mg::BufferID last_client_id;

    for (int i = 0; i < 100; i++)
    {
        for (int j = 0; j < 100; j++)
        {
            auto client = bundle.client_acquire();
            last_client_id = client->id();
            bundle.client_release(client);
        }

        auto compositor = bundle.compositor_acquire();
        ASSERT_EQ(last_client_id, compositor->id());
        bundle.compositor_release(compositor);
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_basic)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);

    auto client = bundle.client_acquire();
    auto client_id = client->id();
    bundle.client_release(client);

    auto compositor = bundle.compositor_acquire();
    EXPECT_EQ(client_id, compositor->id());
    bundle.compositor_release(compositor); 
}

TEST_F(SwitchingBundleTest, compositor_acquire_never_blocks)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);
    const int N = 100;

    bundle.force_requests_to_complete();

    std::shared_ptr<mg::Buffer> buf[N];
    for (int i = 0; i < N; i++)
        buf[i] = bundle.compositor_acquire();

    for (int i = 0; i < N; i++)
        bundle.compositor_release(buf[i]);
}

TEST_F(SwitchingBundleTest, compositor_acquire_recycles_latest_ready_buffer)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);
    const int N = 100;

    mg::BufferID client_id;

    for (int i = 0; i < N; i++)
    {
        if (i % 10 == 0)
        {
            auto client = bundle.client_acquire();
            client_id = client->id();
            bundle.client_release(client);
        }
        auto compositor = bundle.compositor_acquire();
        ASSERT_EQ(client_id, compositor->id());
        bundle.compositor_release(compositor);
    }
}

