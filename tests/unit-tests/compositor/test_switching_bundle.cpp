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

    for (int nbuffers = 1; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, properties);
    
        EXPECT_FALSE(bundle.framedropping_allowed());
        EXPECT_EQ(properties, bundle.properties());
    }
}

TEST_F(SwitchingBundleTest, invalid_nbuffers)
{
    EXPECT_THROW(
        mc::SwitchingBundle a(0, allocator, basic_properties),
        std::logic_error
    );

    EXPECT_THROW(
        mc::SwitchingBundle b(-1, allocator, basic_properties),
        std::logic_error
    );

    EXPECT_THROW(
        mc::SwitchingBundle c(-123, allocator, basic_properties),
        std::logic_error
    );
}

TEST_F(SwitchingBundleTest, client_acquire_basic)
{
    for (int nbuffers = 1; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        auto buffer = bundle.client_acquire();
        bundle.client_release(buffer); 
    }
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
    for (int nbuffers = 1; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
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
            
            if (i >= 2 && nbuffers == 2)
                ASSERT_EQ(composited_id, prev_prev_id);
    
            if (i >= 1 && nbuffers > 1)
                ASSERT_NE(composited_id, prev_id);
    
            prev_prev_id = prev_id;
            prev_id = composited_id;
        }
    }
}

TEST_F(SwitchingBundleTest, framedropping_clients_never_block)
{
    for (int nbuffers = 2; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
    
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
}

TEST_F(SwitchingBundleTest, out_of_order_client_release)
{
    for (int nbuffers = 2; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
    
        auto client1 = bundle.client_acquire();
        auto client2 = bundle.client_acquire();
        EXPECT_THROW(
            bundle.client_release(client2),
            std::logic_error
        );

        bundle.client_release(client1);
        EXPECT_THROW(
            bundle.client_release(client1),
            std::logic_error
        );
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_basic)
{
    for (int nbuffers = 1; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
    
        auto client = bundle.client_acquire();
        auto client_id = client->id();
        bundle.client_release(client);
    
        auto compositor = bundle.compositor_acquire();
        EXPECT_EQ(client_id, compositor->id());
        bundle.compositor_release(compositor); 
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_never_blocks)
{
    for (int nbuffers = 1; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        const int N = 100;
    
        bundle.force_requests_to_complete();
    
        std::shared_ptr<mg::Buffer> buf[N];
        for (int i = 0; i < N; i++)
            buf[i] = bundle.compositor_acquire();
    
        for (int i = 0; i < N; i++)
            bundle.compositor_release(buf[i]);
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_recycles_latest_ready_buffer)
{
    for (int nbuffers = 1; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
    
        mg::BufferID client_id;
    
        for (int i = 0; i < 100; i++)
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
}

TEST_F(SwitchingBundleTest, out_of_order_compositor_release)
{
    for (int nbuffers = 2; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
    
        bundle.client_release(bundle.client_acquire());
        auto compositor1 = bundle.compositor_acquire();

        bundle.client_release(bundle.client_acquire());
        auto compositor2 = bundle.compositor_acquire();

        EXPECT_THROW(
            bundle.compositor_release(compositor2),
            std::logic_error
        );

        bundle.compositor_release(compositor1);
        EXPECT_THROW(
            bundle.compositor_release(compositor1),
            std::logic_error
        );
    }
}

TEST_F(SwitchingBundleTest, overlapping_compositors_get_different_frames)
{
    // This test simulates bypass behaviour
    for (int nbuffers = 2; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
    
        std::shared_ptr<mg::Buffer> compositor[2];
    
        bundle.client_release(bundle.client_acquire());
        compositor[0] = bundle.compositor_acquire();
    
        bundle.client_release(bundle.client_acquire());
        compositor[1] = bundle.compositor_acquire();
    
        for (int i = 0; i < 100; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_NE(compositor[0]->id(), compositor[1]->id());
    
            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            bundle.compositor_release(compositor[oldest]);
            bundle.client_release(bundle.client_acquire());
            compositor[oldest] = bundle.compositor_acquire();
        }
    
        bundle.compositor_release(compositor[0]);
        bundle.compositor_release(compositor[1]);
    }
}

TEST_F(SwitchingBundleTest, no_lag_from_client_to_compositor)
{   // Regression test for input lag bug LP: #1199450
    for (int nbuffers = 2; nbuffers < 10; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        bundle.allow_framedropping(false);
        for (int i = 0; i < nbuffers * 3; i++)
        {
            auto client = bundle.client_acquire();
            auto client_id = client->id();
            bundle.client_release(client);

            auto compositor = bundle.compositor_acquire();
            ASSERT_EQ(client_id, compositor->id());
            bundle.compositor_release(compositor);
        }

        mg::BufferID client_id;
        bundle.allow_framedropping(true);
        for (int i = 0; i < nbuffers * 3; i++)
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

