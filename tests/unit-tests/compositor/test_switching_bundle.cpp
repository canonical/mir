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

#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>

namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mg = mir::graphics;

using namespace testing;

struct SwitchingBundleTest : public ::testing::Test
{
    void SetUp()
    {
        allocator = std::make_shared<mtd::StubBufferAllocator>();
        basic_properties =
        {
            geom::Size{3, 4},
            mir_pixel_format_abgr_8888,
            mg::BufferUsage::hardware
        };
    }

    std::shared_ptr<mtd::StubBufferAllocator> allocator;

    mg::BufferProperties basic_properties;
};

TEST_F(SwitchingBundleTest, sync_swapper_by_default)
{
    mg::BufferProperties properties{geom::Size{7, 8},
                                    mir_pixel_format_argb_8888,
                                    mg::BufferUsage::software};

    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
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

    EXPECT_THROW(
        mc::SwitchingBundle d(10, allocator, basic_properties),
        std::logic_error
    );
}

TEST_F(SwitchingBundleTest, client_acquire_basic)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        auto buffer = bundle.client_acquire();
        bundle.client_release(buffer);
    }
}

namespace
{
    void composite_thread(mc::SwitchingBundle &bundle,
                          unsigned long &frameno,
                          mg::BufferID &composited)
    {
        frameno++;
        auto buffer = bundle.compositor_acquire(frameno);
        composited = buffer->id();
        bundle.compositor_release(buffer);
    }
}

TEST_F(SwitchingBundleTest, is_really_synchronous)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        mg::BufferID prev_id, prev_prev_id;
        unsigned long frameno = 0;

        ASSERT_FALSE(bundle.framedropping_allowed());

        for (int i = 0; i < 20; i++)
        {
            auto client1 = bundle.client_acquire();
            mg::BufferID expect_id = client1->id(), composited_id;
            bundle.client_release(client1);

            std::thread compositor(composite_thread,
                                   std::ref(bundle),
                                   std::ref(frameno),
                                   std::ref(composited_id));

            compositor.join();
            ASSERT_EQ(expect_id, composited_id);

            if (i >= 2 && nbuffers == 2)
                ASSERT_EQ(composited_id, prev_prev_id);

            prev_prev_id = prev_id;
            prev_id = composited_id;

            auto second_monitor = bundle.compositor_acquire(frameno);
            ASSERT_EQ(composited_id, second_monitor->id());
            bundle.compositor_release(second_monitor);
        }
    }
}

TEST_F(SwitchingBundleTest, framedropping_clients_never_block)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned int frameno = 0;

        bundle.allow_framedropping(true);
        mg::BufferID last_client_id;

        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 100; j++)
            {
                auto client = bundle.client_acquire();
                last_client_id = client->id();
                bundle.client_release(client);
            }

            // Flush the pipeline of previously ready buffers
            for (int k = 0; k < nbuffers-1; k++)
            {
                frameno++;
                bundle.compositor_release(bundle.compositor_acquire(frameno));
            }

            frameno++;
            auto compositor = bundle.compositor_acquire(frameno);
            ASSERT_EQ(last_client_id, compositor->id());
            bundle.compositor_release(compositor);
        }
    }
}

TEST_F(SwitchingBundleTest, clients_dont_recycle_startup_buffer)
{  // Regression test for LP: #1210042
    mc::SwitchingBundle bundle(3, allocator, basic_properties);

    auto client1 = bundle.client_acquire();
    auto client1_id = client1->id();
    bundle.client_release(client1);

    auto client2 = bundle.client_acquire();
    bundle.client_release(client2);

    auto compositor = bundle.compositor_acquire(1);
    EXPECT_EQ(client1_id, compositor->id());
    bundle.compositor_release(compositor);
}

TEST_F(SwitchingBundleTest, out_of_order_client_release)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
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
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 0;

        auto client = bundle.client_acquire();
        auto client_id = client->id();
        bundle.client_release(client);

        for (int monitor = 0; monitor < 10; monitor++)
        {
            frameno++;
            auto compositor = bundle.compositor_acquire(frameno);
            ASSERT_EQ(client_id, compositor->id());
            bundle.compositor_release(compositor);
        }
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_never_blocks)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 0;
        const int N = 100;

        bundle.force_requests_to_complete();

        std::shared_ptr<mg::Buffer> buf[N];
        for (int i = 0; i < N; i++)
            buf[i] = bundle.compositor_acquire(++frameno);

        for (int i = 0; i < N; i++)
            bundle.compositor_release(buf[i]);
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_recycles_latest_ready_buffer)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 1;

        mg::BufferID client_id;

        for (int i = 0; i < 20; i++)
        {
            if (i % 10 == 0)
            {
                auto client = bundle.client_acquire();
                client_id = client->id();
                bundle.client_release(client);
            }

            for (int monitor_id = 0; monitor_id < 10; monitor_id++)
            {
                auto compositor = bundle.compositor_acquire(frameno);
                ASSERT_EQ(client_id, compositor->id());
                bundle.compositor_release(compositor);
            }

            frameno++;
        }
    }
}

TEST_F(SwitchingBundleTest, compositor_release_verifies_parameter)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 0;

        auto client = bundle.client_acquire();
        bundle.client_release(client);

        auto compositor1 = bundle.compositor_acquire(++frameno);
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
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 1;

        std::shared_ptr<mg::Buffer> compositor[2];

        bundle.client_release(bundle.client_acquire());
        compositor[0] = bundle.compositor_acquire(frameno);

        frameno++;
        bundle.client_release(bundle.client_acquire());
        compositor[1] = bundle.compositor_acquire(frameno);

        for (int i = 0; i < 20; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_NE(compositor[0]->id(), compositor[1]->id());

            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            bundle.compositor_release(compositor[oldest]);
            bundle.client_release(bundle.client_acquire());
            frameno++;
            compositor[oldest] = bundle.compositor_acquire(frameno);
        }

        bundle.compositor_release(compositor[0]);
        bundle.compositor_release(compositor[1]);
    }
}

TEST_F(SwitchingBundleTest, snapshot_acquire_basic)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        auto compositor = bundle.compositor_acquire(1);
        auto snapshot = bundle.snapshot_acquire();
        EXPECT_EQ(snapshot->id(), compositor->id());
        bundle.compositor_release(compositor);
        bundle.snapshot_release(snapshot);
    }
}

TEST_F(SwitchingBundleTest, snapshot_acquire_never_blocks)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        const int N = 100;

        std::shared_ptr<mg::Buffer> buf[N];
        for (int i = 0; i < N; i++)
            buf[i] = bundle.snapshot_acquire();

        for (int i = 0; i < N; i++)
            bundle.snapshot_release(buf[i]);
    }
}

TEST_F(SwitchingBundleTest, snapshot_release_verifies_parameter)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        auto compositor = bundle.compositor_acquire(1);

        EXPECT_THROW(
            bundle.snapshot_release(compositor),
            std::logic_error
        );

        auto client = bundle.client_acquire();
        auto snapshot = bundle.snapshot_acquire();

        EXPECT_EQ(compositor->id(), snapshot->id());

        EXPECT_NE(client->id(), snapshot->id());

        bundle.snapshot_release(snapshot);

        EXPECT_THROW(
            bundle.snapshot_release(snapshot),
            std::logic_error
        );
    }
}

namespace
{
    void compositor_thread(mc::SwitchingBundle &bundle,
                           std::atomic<bool> &done)
    {
        unsigned long frameno = 0;
        while (!done)
        {
            bundle.compositor_release(bundle.compositor_acquire(++frameno));
            std::this_thread::yield();
        }
    }

    void snapshot_thread(mc::SwitchingBundle &bundle,
                           std::atomic<bool> &done)
    {
        while (!done)
        {
            bundle.snapshot_release(bundle.snapshot_acquire());
            std::this_thread::yield();
        }
    }

    void client_thread(mc::SwitchingBundle &bundle, int nframes)
    {
        for (int i = 0; i < nframes; i++)
        {
            bundle.client_release(bundle.client_acquire());
            std::this_thread::yield();
        }
    }

    void switching_client_thread(mc::SwitchingBundle &bundle, int nframes)
    {
        for (int i = 0; i < nframes; i += 10)
        {
            bundle.allow_framedropping(false);
            for (int j = 0; j < 5; j++)
                bundle.client_release(bundle.client_acquire());
            std::this_thread::yield();

            bundle.allow_framedropping(true);
            for (int j = 0; j < 5; j++)
                bundle.client_release(bundle.client_acquire());
            std::this_thread::yield();
        }
    }
}

TEST_F(SwitchingBundleTest, stress)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        std::atomic<bool> done;
        done = false;

        std::thread compositor(compositor_thread,
                               std::ref(bundle),
                               std::ref(done));
        std::thread snapshotter1(snapshot_thread,
                                std::ref(bundle),
                                std::ref(done));
        std::thread snapshotter2(snapshot_thread,
                                std::ref(bundle),
                                std::ref(done));

        bundle.allow_framedropping(false);
        std::thread client1(client_thread, std::ref(bundle), 1000);
        client1.join();

        bundle.allow_framedropping(true);
        std::thread client2(client_thread, std::ref(bundle), 1000);
        client2.join();

        std::thread client3(switching_client_thread, std::ref(bundle), 1000);
        client3.join();

        done = true;

        compositor.join();
        snapshotter1.join();
        snapshotter2.join();
    }
}

// FIXME (enabling this optimization breaks timing tests)
TEST_F(SwitchingBundleTest, DISABLED_synchronous_clients_only_get_two_real_buffers)
{
    /*
     * You might ask for more buffers, but we should only allocate two
     * unique ones while it makes sense to do so. Buffers are big things and
     * should be allocated sparingly...
     */
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 0;

        bundle.allow_framedropping(false);

        const int nframes = 100;
        mg::BufferID prev_id, prev_prev_id;

        std::thread client(client_thread, std::ref(bundle), nframes);

        for (int frame = 0; frame < nframes; frame++)
        {
            frameno++;

            auto compositor = bundle.compositor_acquire(frameno);
            auto compositor_id = compositor->id();
            bundle.compositor_release(compositor);

            if (frame >= 2)
                ASSERT_EQ(prev_prev_id, compositor_id);

            prev_prev_id = prev_id;
            prev_id = compositor_id;
        }

        client.join();
    }
}

TEST_F(SwitchingBundleTest, bypass_clients_get_more_than_two_buffers)
{
    for (int nbuffers = 3;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 1;

        std::shared_ptr<mg::Buffer> compositor[2];

        bundle.client_release(bundle.client_acquire());
        compositor[0] = bundle.compositor_acquire(frameno);

        frameno++;
        bundle.client_release(bundle.client_acquire());
        compositor[1] = bundle.compositor_acquire(frameno);

        for (int i = 0; i < 20; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_NE(compositor[0]->id(), compositor[1]->id());

            auto client = bundle.client_acquire();
            ASSERT_NE(compositor[0]->id(), client->id());
            ASSERT_NE(compositor[1]->id(), client->id());
            bundle.client_release(client);

            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            bundle.compositor_release(compositor[oldest]);

            frameno++;
            compositor[oldest] = bundle.compositor_acquire(frameno);
        }

        bundle.compositor_release(compositor[0]);
        bundle.compositor_release(compositor[1]);
    }
}

TEST_F(SwitchingBundleTest, framedropping_clients_get_all_buffers)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        bundle.allow_framedropping(true);

        const int nframes = 100;
        mg::BufferID expect[mc::SwitchingBundle::max_buffers];
        mg::Buffer* buf[mc::SwitchingBundle::max_buffers];

        for (int b = 0; b < nbuffers; b++)
        {
            buf[b] = bundle.client_acquire();
            expect[b] = buf[b]->id();

            for (int p = 0; p < b; p++)
                ASSERT_NE(expect[p], expect[b]);
        }

        for (int b = 0; b < nbuffers; b++)
            bundle.client_release(buf[b]);

        for (int frame = 0; frame < nframes; frame++)
        {
            auto client = bundle.client_acquire();
            ASSERT_EQ(expect[frame % nbuffers], client->id());
            bundle.client_release(client);
        }
    }
}

TEST_F(SwitchingBundleTest, waiting_clients_unblock_on_shutdown)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        bundle.allow_framedropping(false);

        std::thread client(client_thread, std::ref(bundle), nbuffers);

        /*
         * Tecnhically we would like to distinguish between final shutdown
         * and temporary shutdown (VT switch). The former should permanently
         * unblock clients. The latter only temporarily unblock clients.
         * But that requires interface changes all over the place...
         */
        bundle.force_requests_to_complete();
        client.join();
    }
}

TEST_F(SwitchingBundleTest, waiting_clients_unblock_on_vt_switch_not_permanent)
{   // Regression test for LP: #1207226
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);

        bundle.allow_framedropping(false);

        std::thread client(client_thread, std::ref(bundle), nbuffers);
        bundle.force_requests_to_complete();
        client.join();

        EXPECT_FALSE(bundle.framedropping_allowed());
    }
}

TEST_F(SwitchingBundleTest, client_framerate_matches_compositor)
{
    for (int nbuffers = 2; nbuffers <= 3; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long client_frames = 0;
        const unsigned long compose_frames = 20;

        bundle.allow_framedropping(false);

        std::atomic<bool> done(false);

        std::thread monitor1([&]
        {
            for (unsigned long frame = 0; frame != compose_frames+3; frame++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                auto buf = bundle.compositor_acquire(frame);
                bundle.compositor_release(buf);

                if (frame == compose_frames)
                {
                    // Tell the "client" to stop after compose_frames, but
                    // don't stop rendering immediately to avoid blocking
                    // if we rendered any twice
                    done.store(true);
                }
            }
        });

        bundle.client_release(bundle.client_acquire());

        while (!done.load())
        {
            bundle.client_release(bundle.client_acquire());
            client_frames++;
        }

        monitor1.join();

        // Roughly compose_frames == client_frames within 50%
        ASSERT_GT(client_frames, compose_frames / 2);
        ASSERT_LT(client_frames, compose_frames * 3 / 2);
    }
}

TEST_F(SwitchingBundleTest, slow_client_framerate_matches_compositor)
{  // Regression test LP: #1241369 / LP: #1241371
    for (int nbuffers = 2; nbuffers <= 3; nbuffers++)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long client_frames = 0;
        const unsigned long compose_frames = 100;
        const auto frame_time = std::chrono::milliseconds(16);

        bundle.allow_framedropping(false);

        std::atomic<bool> done(false);
        std::mutex sync;

        std::thread monitor1([&]
        {
            for (unsigned long frame = 0; frame != compose_frames+3; frame++)
            {
                std::this_thread::sleep_for(frame_time);
                sync.lock();
                auto buf = bundle.compositor_acquire(frame);
                bundle.compositor_release(buf);
                sync.unlock();

                if (frame == compose_frames)
                {
                    // Tell the "client" to stop after compose_frames, but
                    // don't stop rendering immediately to avoid blocking
                    // if we rendered any twice
                    done.store(true);
                }
            }
        });

        bundle.client_release(bundle.client_acquire());

        while (!done.load())
        {
            sync.lock();
            sync.unlock();
            auto buf = bundle.client_acquire();
            std::this_thread::sleep_for(frame_time);
            bundle.client_release(buf);
            client_frames++;
        }

        monitor1.join();

        // Roughly compose_frames == client_frames within 20%
        ASSERT_GT(client_frames, compose_frames * 0.8f);
        ASSERT_LT(client_frames, compose_frames * 1.2f);
    }
}

TEST_F(SwitchingBundleTest, resize_affects_client_acquires_immediately)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        unsigned long frameno = 1;

        for (int width = 1; width < 100; ++width)
        {
            const geom::Size expect_size{width, width * 2};

            for (int subframe = 0; subframe < 3; ++subframe)
            {
                bundle.resize(expect_size);
                auto client = bundle.client_acquire();
                ASSERT_EQ(expect_size, client->size());
                bundle.client_release(client);

                auto compositor = bundle.compositor_acquire(frameno++);
                ASSERT_EQ(expect_size, compositor->size());
                bundle.compositor_release(compositor);
            }
        }
    }
}

TEST_F(SwitchingBundleTest, compositor_acquires_resized_frames)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties);
        mg::BufferID history[5];
        unsigned long frameno = 1;

        const int width0 = 123;
        const int height0 = 456;
        const int dx = 2;
        const int dy = -3;
        int width = width0;
        int height = height0;

        for (int produce = 0; produce < nbuffers; ++produce)
        {
            geom::Size new_size{width, height};
            width += dx;
            height += dy;

            bundle.resize(new_size);
            auto client = bundle.client_acquire();
            history[produce] = client->id();
            ASSERT_EQ(new_size, client->size());
            bundle.client_release(client);
        }

        width = width0;
        height = height0;

        for (int consume = 0; consume < nbuffers; ++consume)
        {
            geom::Size expect_size{width, height};
            width += dx;
            height += dy;

            auto compositor = bundle.compositor_acquire(frameno++);

            // Verify the compositor gets resized buffers, eventually
            ASSERT_EQ(expect_size, compositor->size());

            // Verify the compositor gets buffers with *contents*, ie. that
            // they have not been resized prematurely and are empty.
            ASSERT_EQ(history[consume], compositor->id());

            bundle.compositor_release(compositor);
        }

        // Verify the final buffer size sticks
        const geom::Size final_size{width - dx, height - dy};
        for (int unchanging = 0; unchanging < 100; ++unchanging)
        {
            auto compositor = bundle.compositor_acquire(frameno++);
            ASSERT_EQ(final_size, compositor->size());
            bundle.compositor_release(compositor);
        }
    }
}

