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
#include "mir/asio_main_loop.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_framework/semaphore.h"
#include "mir_test_framework/watchdog.h"

#include <gtest/gtest.h>

#include <atomic>
#include <thread>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <map>

namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;
namespace mg = mir::graphics;
namespace mtf=mir_test_framework;

using namespace testing;

namespace
{
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
        timer = std::make_shared<mir::AsioMainLoop>();
        mainloop_thread = std::thread([this]() {timer->run();});
    }

    void TearDown()
    {
        timer->stop();
        if (mainloop_thread.joinable())
            mainloop_thread.join();
    }

    std::shared_ptr<mtd::StubBufferAllocator> allocator;

    mg::BufferProperties basic_properties;
    std::shared_ptr<mir::AsioMainLoop> timer;
    std::thread mainloop_thread;
};

auto client_acquire_cancellable(mc::SwitchingBundle& switching_bundle, std::shared_ptr<mtf::Semaphore> signal)
-> mg::Buffer*
{
    mg::Buffer* result;
    switching_bundle.client_acquire(
                [&, signal](mg::Buffer* new_buffer)
    {
        result = new_buffer;
        signal->raise();
    });

    signal->wait();

    return result;
}

auto client_acquire_blocking(mc::SwitchingBundle& switching_bundle)
-> mg::Buffer*
{
    return client_acquire_cancellable(switching_bundle, std::make_shared<mtf::Semaphore>());
}
}

TEST_F(SwitchingBundleTest, sync_swapper_by_default)
{
    mg::BufferProperties properties{geom::Size{7, 8},
                                    mir_pixel_format_argb_8888,
                                    mg::BufferUsage::software};

    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, properties, timer);

        EXPECT_FALSE(bundle.framedropping_allowed());
        EXPECT_EQ(properties, bundle.properties());
    }
}

TEST_F(SwitchingBundleTest, invalid_nbuffers)
{
    EXPECT_THROW(
        mc::SwitchingBundle a(0, allocator, basic_properties, timer),
        std::logic_error
    );

    EXPECT_THROW(
        mc::SwitchingBundle b(-1, allocator, basic_properties, timer),
        std::logic_error
    );

    EXPECT_THROW(
        mc::SwitchingBundle c(-123, allocator, basic_properties, timer),
        std::logic_error
    );

    EXPECT_THROW(
        mc::SwitchingBundle d(10, allocator, basic_properties, timer),
        std::logic_error
    );
}

TEST_F(SwitchingBundleTest, client_acquire_basic)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto buffer = client_acquire_blocking(bundle);
        bundle.client_release(buffer);
    }
}

namespace
{
    void composite_thread(mc::SwitchingBundle &bundle,
                          mg::BufferID &composited)
    {
        auto buffer = bundle.compositor_acquire(nullptr);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
        mg::BufferID prev_id, prev_prev_id;

        ASSERT_FALSE(bundle.framedropping_allowed());

        for (int i = 0; i < 20; i++)
        {
            auto client1 = client_acquire_blocking(bundle);
            mg::BufferID expect_id = client1->id(), composited_id;
            bundle.client_release(client1);

            std::thread compositor(composite_thread,
                                   std::ref(bundle),
                                   std::ref(composited_id));

            compositor.join();
            ASSERT_EQ(expect_id, composited_id);

            if (i >= 2 && nbuffers == 2)
                ASSERT_EQ(composited_id, prev_prev_id);

            prev_prev_id = prev_id;
            prev_id = composited_id;

            auto second_monitor = bundle.compositor_acquire(this);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        bundle.allow_framedropping(true);
        mg::BufferID last_client_id;

        for (int i = 0; i < 10; i++)
        {
            for (int j = 0; j < 100; j++)
            {
                auto client = client_acquire_blocking(bundle);
                last_client_id = client->id();
                bundle.client_release(client);
            }

            // Flush the pipeline of previously ready buffers
            for (int k = 0; k < nbuffers-1; k++)
            {
                bundle.compositor_release(bundle.compositor_acquire(nullptr));
            }

            auto compositor = bundle.compositor_acquire(nullptr);
            ASSERT_EQ(last_client_id, compositor->id());
            bundle.compositor_release(compositor);
        }
    }
}

TEST_F(SwitchingBundleTest, clients_dont_recycle_startup_buffer)
{  // Regression test for LP: #1210042
    mc::SwitchingBundle bundle(3, allocator, basic_properties, timer);

    auto client1 = client_acquire_blocking(bundle);
    auto client1_id = client1->id();
    bundle.client_release(client1);

    auto client2 = client_acquire_blocking(bundle);
    bundle.client_release(client2);

    auto compositor = bundle.compositor_acquire(nullptr);
    EXPECT_EQ(client1_id, compositor->id());
    bundle.compositor_release(compositor);
}

TEST_F(SwitchingBundleTest, out_of_order_client_release)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto client1 = client_acquire_blocking(bundle);
        auto client2 = client_acquire_blocking(bundle);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto client = client_acquire_blocking(bundle);
        auto client_id = client->id();
        bundle.client_release(client);

        auto compositor = bundle.compositor_acquire(nullptr);
        ASSERT_EQ(client_id, compositor->id());
        bundle.compositor_release(compositor);
    }
}

TEST_F(SwitchingBundleTest, multimonitor_frame_sync)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto client = client_acquire_blocking(bundle);
        auto client_id = client->id();
        bundle.client_release(client);

        for (int monitor = 0; monitor < 10; monitor++)
        {
            void const* user_id = reinterpret_cast<void const*>(monitor);
            auto compositor = bundle.compositor_acquire(user_id);
            ASSERT_EQ(client_id, compositor->id());
            bundle.compositor_release(compositor);
        }
    }
}

TEST_F(SwitchingBundleTest, frames_in_order)
{
    int const nbuffers = 3;
    mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

    mg::Buffer* clients[nbuffers];
    for (auto& client : clients)
    {
        client = client_acquire_blocking(bundle);
        bundle.client_release(client);
    }

    for (int frame = 0; frame < nbuffers; ++frame)
    {
        if (frame > 0)
            ASSERT_NE(clients[frame-1]->id(), clients[frame]->id());

        auto compositor = bundle.compositor_acquire(nullptr);
        ASSERT_EQ(clients[frame]->id(), compositor->id());
        bundle.compositor_release(compositor);
    }
}

TEST_F(SwitchingBundleTest, compositor_acquire_never_blocks)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
        const int N = 100;

        bundle.force_requests_to_complete();

        std::shared_ptr<mg::Buffer> buf[N];
        for (int i = 0; i < N; i++)
            buf[i] = bundle.compositor_acquire(nullptr);

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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        mg::BufferID client_id;

        for (int i = 0; i < 20; i++)
        {
            if (i % 10 == 0)
            {
                auto client = client_acquire_blocking(bundle);
                client_id = client->id();
                bundle.client_release(client);
            }

            for (int monitor_id = 0; monitor_id < 10; monitor_id++)
            {
                void const* user_id =
                    reinterpret_cast<void const*>(monitor_id);
                auto compositor = bundle.compositor_acquire(user_id);
                ASSERT_EQ(client_id, compositor->id());
                bundle.compositor_release(compositor);
            }
        }
    }
}

TEST_F(SwitchingBundleTest, compositor_release_verifies_parameter)
{
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto client = client_acquire_blocking(bundle);
        bundle.client_release(client);

        auto compositor1 = bundle.compositor_acquire(nullptr);
        bundle.compositor_release(compositor1);
        EXPECT_THROW(
            bundle.compositor_release(compositor1),
            std::logic_error
        );
    }
}

/*
 Regression test for LP#1270964
 In the original bug, SwitchingBundle::last_consumed would be used without ever being set
 in the compositor_acquire() call, thus its initial value of zero would wrongly be used
 and lead to the wrong buffer being given to the compositor
 */
TEST_F(SwitchingBundleTest, compositor_client_interleaved)
{
    int nbuffers = 3;
    mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
    mg::Buffer* client_buffer = nullptr;
    std::shared_ptr<mg::Buffer> compositor_buffer = nullptr;

    client_buffer = client_acquire_blocking(bundle);
    mg::BufferID first_ready_buffer_id = client_buffer->id();
    bundle.client_release(client_buffer);

    client_buffer = client_acquire_blocking(bundle);

    // in the original bug, compositor would be given the wrong buffer here
    compositor_buffer = bundle.compositor_acquire(0);

    ASSERT_EQ(first_ready_buffer_id, compositor_buffer->id());

    // Clean up
    bundle.client_release(client_buffer);
    bundle.compositor_release(compositor_buffer);
    compositor_buffer.reset();
}

TEST_F(SwitchingBundleTest, overlapping_compositors_get_different_frames)
{
    // This test simulates bypass behaviour
    for (int nbuffers = 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         ++nbuffers)
    {
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        std::shared_ptr<mg::Buffer> compositor[2];

        bundle.client_release(client_acquire_blocking(bundle));
        compositor[0] = bundle.compositor_acquire(nullptr);

        bundle.client_release(client_acquire_blocking(bundle));
        compositor[1] = bundle.compositor_acquire(nullptr);

        for (int i = 0; i < 20; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_NE(compositor[0]->id(), compositor[1]->id());

            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            bundle.compositor_release(compositor[oldest]);
            bundle.client_release(client_acquire_blocking(bundle));
            compositor[oldest] = bundle.compositor_acquire(nullptr);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto compositor = bundle.compositor_acquire(nullptr);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        auto compositor = bundle.compositor_acquire(nullptr);

        EXPECT_THROW(
            bundle.snapshot_release(compositor),
            std::logic_error
        );

        auto client = client_acquire_blocking(bundle);
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
        while (!done)
        {
            bundle.compositor_release(bundle.compositor_acquire(nullptr));
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
            bundle.client_release(client_acquire_blocking(bundle));
            std::this_thread::yield();
        }
    }

    void switching_client_thread(mc::SwitchingBundle &bundle, int nframes)
    {
        for (int i = 0; i < nframes; i += 10)
        {
            bundle.allow_framedropping(false);
            for (int j = 0; j < 5; j++)
                bundle.client_release(client_acquire_blocking(bundle));
            std::this_thread::yield();

            bundle.allow_framedropping(true);
            for (int j = 0; j < 5; j++)
                bundle.client_release(client_acquire_blocking(bundle));
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        bundle.allow_framedropping(false);

        const int nframes = 100;
        mg::BufferID prev_id, prev_prev_id;

        std::thread client(client_thread, std::ref(bundle), nframes);

        for (int frame = 0; frame < nframes; frame++)
        {
            auto compositor = bundle.compositor_acquire(nullptr);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        std::shared_ptr<mg::Buffer> compositor[2];

        bundle.client_release(client_acquire_blocking(bundle));
        compositor[0] = bundle.compositor_acquire(nullptr);

        bundle.client_release(client_acquire_blocking(bundle));
        compositor[1] = bundle.compositor_acquire(nullptr);

        for (int i = 0; i < 20; i++)
        {
            // Two compositors acquired, and they're always different...
            ASSERT_NE(compositor[0]->id(), compositor[1]->id());

            auto client = client_acquire_blocking(bundle);
            ASSERT_NE(compositor[0]->id(), client->id());
            ASSERT_NE(compositor[1]->id(), client->id());
            bundle.client_release(client);

            // One of the compositors (the oldest one) gets a new buffer...
            int oldest = i & 1;
            bundle.compositor_release(compositor[oldest]);

            compositor[oldest] = bundle.compositor_acquire(nullptr);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        bundle.allow_framedropping(true);

        const int nframes = 100;
        mg::BufferID expect[mc::SwitchingBundle::max_buffers];
        mg::Buffer* buf[mc::SwitchingBundle::max_buffers];

        for (int b = 0; b < nbuffers; b++)
        {
            buf[b] = client_acquire_blocking(bundle);
            expect[b] = buf[b]->id();

            for (int p = 0; p < b; p++)
                ASSERT_NE(expect[p], expect[b]);
        }

        for (int b = 0; b < nbuffers; b++)
            bundle.client_release(buf[b]);

        for (int frame = 0; frame < nframes; frame++)
        {
            auto client = client_acquire_blocking(bundle);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
        unsigned long client_frames = 0;
        const unsigned long compose_frames = 20;

        bundle.allow_framedropping(false);

        std::atomic<bool> done(false);

        std::thread monitor1([&]
        {
            for (unsigned long frame = 0; frame != compose_frames+3; frame++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(10));

                auto buf = bundle.compositor_acquire(this);
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

        bundle.client_release(client_acquire_blocking(bundle));

        while (!done.load())
        {
            bundle.client_release(client_acquire_blocking(bundle));
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
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
                auto buf = bundle.compositor_acquire(this);
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

        bundle.client_release(client_acquire_blocking(bundle));

        while (!done.load())
        {
            sync.lock();
            sync.unlock();
            auto buf = client_acquire_blocking(bundle);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);

        for (int width = 1; width < 100; ++width)
        {
            const geom::Size expect_size{width, width * 2};

            for (int subframe = 0; subframe < 3; ++subframe)
            {
                bundle.resize(expect_size);
                auto client = client_acquire_blocking(bundle);
                ASSERT_EQ(expect_size, client->size());
                bundle.client_release(client);

                auto compositor = bundle.compositor_acquire(nullptr);
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
        mc::SwitchingBundle bundle(nbuffers, allocator, basic_properties, timer);
        mg::BufferID history[5];

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
            auto client = client_acquire_blocking(bundle);
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

            auto compositor = bundle.compositor_acquire(nullptr);

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
            auto compositor = bundle.compositor_acquire(nullptr);
            ASSERT_EQ(final_size, compositor->size());
            bundle.compositor_release(compositor);
        }
    }
}

TEST_F(SwitchingBundleTest, uncomposited_client_swaps_at_given_rate)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers;
         nbuffers < mc::SwitchingBundle::max_buffers;
         nbuffers++)
    {
        for (auto block_time = std::chrono::milliseconds{100};
             block_time < std::chrono::milliseconds{500};
             block_time += std::chrono::milliseconds{100})
        {
            mc::SwitchingBundle bundle(nbuffers,
                                       allocator,
                                       basic_properties,
                                       timer,
                                       block_time);

            for (int i = 0; i < nbuffers; i++)
            {
                auto client = client_acquire_blocking(bundle);
                bundle.client_release(client);
            }
            auto signal = std::make_shared<mtf::Semaphore>();
            mtf::WatchDog watchdog{[signal]() { signal->raise(); }};

            auto start = std::chrono::high_resolution_clock::now();

            watchdog.run([&bundle, signal](mtf::WatchDog& watchdog)
            {
                client_acquire_cancellable(bundle, signal);
                watchdog.notify_done();
            });
            // Give 100ms grace time for slow machines running under valgrind...
            EXPECT_TRUE(watchdog.wait_for(block_time + std::chrono::milliseconds{100}));
            EXPECT_GE(std::chrono::high_resolution_clock::now() - start, block_time);
        }
    }
}

TEST_F(SwitchingBundleTest, compositor_swapping_at_min_rate_gets_oldest_buffer)
{
    for (int nbuffers = mc::SwitchingBundle::min_buffers > 1 ? mc::SwitchingBundle::min_buffers : 2;
         nbuffers <= mc::SwitchingBundle::max_buffers;
         nbuffers++)
    {
        // We have the world's fastest display!
        std::chrono::milliseconds const vsync_delay{1};
        int const num_frames{1000};

        mc::SwitchingBundle bundle(nbuffers,
                                   allocator,
                                   basic_properties,
                                   timer,
                                   vsync_delay);

        mtf::Semaphore kill_switch;
        mtf::WatchDog compositor_runner{[&kill_switch](){ kill_switch.raise(); }};
        mtf::WatchDog client_runner{[&kill_switch](){ kill_switch.raise(); }};

        std::mutex buffer_mutex;
        std::map<mg::BufferID, int> buffers_age;
        mg::BufferID racing_buffer;

        compositor_runner.run([&](mtf::WatchDog& watchdog)
        {
            for (int i = 0; !kill_switch.raised(); i++)
            {
                std::shared_ptr<mg::Buffer> compositor;
                {
                    std::lock_guard<decltype(buffer_mutex)> lock(buffer_mutex);
                    compositor = bundle.compositor_acquire(nullptr);
                    if (!buffers_age.empty())
                    {
                        auto max = std::max_element(buffers_age.begin(),
                                                    buffers_age.end(),
                                                    [](std::pair<mg::BufferID, int> const& a,
                                                       std::pair<mg::BufferID, int> const& b)
                        {
                            return a.second < b.second;
                        });

                        // We expect to be compositing the oldest buffer
                        // but there's a small race where the bundle has prepared
                        // a new buffer for the client but has not yet sent it out
                        if (max->first != compositor->id())
                        {
                            // We shouldn't be able to beat the client twice, though...
                            EXPECT_FALSE(racing_buffer.is_valid());
                            // ...and we should have the second-oldest buffer recorded.
                            auto oldest_age = max->second;
                            if (oldest_age == 0)
                            {
                                // ...but for the two-buffer case we can only ever have
                                // buffers of age 0 - one for the compositor, one for
                                // the client.
                                EXPECT_EQ(oldest_age, buffers_age[compositor->id()]);
                            }
                            else
                            {
                                EXPECT_EQ(oldest_age - 1, buffers_age[compositor->id()]);
                            }
                            racing_buffer = max->first;
                        }

                        buffers_age.erase(compositor->id());
                    }
                }
                std::this_thread::sleep_for(vsync_delay);
                bundle.compositor_release(compositor);
                if (i >= num_frames)
                    watchdog.notify_done();
            }
        });

        ASSERT_FALSE(bundle.framedropping_allowed());
        bool has_dropped_frame{false};
        client_runner.run([&](mtf::WatchDog& watchdog)
        {
            for (int i = 0; !kill_switch.raised(); i++)
            {
                auto client = client_acquire_blocking(bundle);
                {
                    std::lock_guard<decltype(buffer_mutex)> lock(buffer_mutex);
                    bundle.client_release(client);

                    if (buffers_age.count(client->id()))
                    {
                        has_dropped_frame = true;
                        if (racing_buffer.is_valid())
                            EXPECT_EQ(racing_buffer, client->id());
                        racing_buffer = mg::BufferID{};
                    }

                    // Age all the things!
                    for (auto& id_age_pair : buffers_age)
                        id_age_pair.second++;
                    // But this one is newborn.
                    buffers_age[client->id()] = 0;
                }
                if (i >= num_frames)
                    watchdog.notify_done();
            }
        });
        EXPECT_TRUE(client_runner.wait_for(std::chrono::milliseconds{1500}));
        EXPECT_TRUE(compositor_runner.wait_for(std::chrono::milliseconds{1500}));
        EXPECT_TRUE(has_dropped_frame);
        compositor_runner.stop();
    }
}
