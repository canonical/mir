/*
 * Copyright © 2013-2015 Canonical Ltd.
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
 *              Alberto Aguirre <alberto.aguirre@canonical.com>
 */

#include "src/server/compositor/buffer_queue.h"
#include "src/server/compositor/timeout_frame_dropping_policy_factory.h"
#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_buffer.h"
#include "mir/test/doubles/stub_frame_dropping_policy_factory.h"
#include "mir/test/doubles/mock_frame_dropping_policy_factory.h"
#include "mir/test/fake_clock.h"
#include "mir/test/doubles/mock_timer.h"
#include "mir/test/signal.h"
#include "mir/test/auto_unblock_thread.h"

#include <gtest/gtest.h>

#include <memory>
#include <atomic>
#include <mutex>
#include <chrono>
#include <algorithm>
#include <map>
#include <deque>
#include <unordered_set>

namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;
namespace mt = mir::test;
namespace mc=mir::compositor;
namespace mg = mir::graphics;
namespace mt=mir::test;

using namespace testing;

namespace
{
int const max_nbuffers_to_test{5};
class BufferQueue : public ::testing::Test
{
public:
    BufferQueue() :
        allocator{std::make_shared<mtd::StubBufferAllocator>()},
        basic_properties{
            geom::Size{3, 4},
            mir_pixel_format_abgr_8888,
            mg::BufferUsage::hardware
        }
    {
    }
protected:
    std::shared_ptr<mtd::StubBufferAllocator> allocator;
    mg::BufferProperties basic_properties;
    mtd::StubFrameDroppingPolicyFactory policy_factory;
};

struct BufferQueueWithOneBuffer : BufferQueue
{
    int nbuffers = 1;
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};
};

struct BufferQueueWithTwoBuffers : BufferQueue
{
    int nbuffers = 2;
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};
};

struct BufferQueueWithThreeBuffers : BufferQueue
{
    int nbuffers = 3;
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};
};

struct WithAnyNumberOfBuffers : BufferQueue, ::testing::WithParamInterface<int>
{
    int nbuffers = GetParam();
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};
};

struct WithTwoOrMoreBuffers : BufferQueue, ::testing::WithParamInterface<int>
{
    int nbuffers = GetParam();
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};
};

struct WithThreeOrMoreBuffers : BufferQueue, ::testing::WithParamInterface<int>
{
    int nbuffers = GetParam();
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};
};

class AcquireWaitHandle
{
public:
    AcquireWaitHandle(mc::BufferQueue& q)
        : buffer_{nullptr}, q{&q}, received_buffer{false}
    {}

    void receive_buffer(mg::Buffer* new_buffer)
    {
        std::lock_guard<decltype(guard)> lock(guard);
        buffer_ = new_buffer;
        received_buffer = true;
        cv.notify_one();
    }

    void wait()
    {
        std::unique_lock<decltype(guard)> lock(guard);
        cv.wait(lock, [&]{ return received_buffer; });
    }

    template<typename Rep, typename Period>
    bool wait_for(std::chrono::duration<Rep, Period> const& duration)
    {
        std::unique_lock<decltype(guard)> lock(guard);
        return cv.wait_for(lock, duration, [&]{ return received_buffer; });
    }

    bool has_acquired_buffer()
    {
        std::lock_guard<decltype(guard)> lock(guard);
        return received_buffer;
    }

    void release_buffer()
    {
        if (buffer_)
        {
            q->client_release(buffer_);
            received_buffer = false;
        }
    }

    mg::BufferID id()
    {
        return buffer_->id();
    }

    mg::Buffer* buffer()
    {
        return buffer_;
    }

private:
    mg::Buffer* buffer_;
    mc::BufferQueue* q;
    std::condition_variable cv;
    std::mutex guard;
    bool received_buffer;
};

std::shared_ptr<AcquireWaitHandle> client_acquire_async(mc::BufferQueue& q)
{
    std::shared_ptr<AcquireWaitHandle> wait_handle =
        std::make_shared<AcquireWaitHandle>(q);

    q.client_acquire(
        [wait_handle](mg::Buffer* buffer) { wait_handle->receive_buffer(buffer); });

    return wait_handle;
}

mg::Buffer* client_acquire_sync(mc::BufferQueue& q)
{
    auto handle = client_acquire_async(q);
    handle->wait();
    return handle->buffer();
}

void unthrottled_compositor_thread(mc::BufferQueue &bundle,
                                   std::atomic<bool> &done)
{
   while (!done)
   {
       bundle.compositor_release(bundle.compositor_acquire(nullptr));
       std::this_thread::yield();
   }
}

void overlapping_compositor_thread(mc::BufferQueue &bundle,
                                   std::atomic<bool> &done)
{
   std::shared_ptr<mg::Buffer> b[2];
   int i = 0;

   b[0] = bundle.compositor_acquire(nullptr);
   while (!done)
   {
       b[i^1] = bundle.compositor_acquire(nullptr);
       bundle.compositor_release(b[i]);
       std::this_thread::sleep_for(std::chrono::milliseconds(10));
       i ^= 1;
   }

   if (b[i])
       bundle.compositor_release(b[i]);
}

void snapshot_thread(mc::BufferQueue &bundle,
                      std::atomic<bool> &done)
{
   while (!done)
   {
       bundle.snapshot_release(bundle.snapshot_acquire());
       std::this_thread::yield();
   }
}

void client_thread(mc::BufferQueue &bundle, int nframes)
{
   for (int i = 0; i < nframes; i++)
   {
       bundle.client_release(client_acquire_sync(bundle));
       std::this_thread::yield();
   }
}

void switching_client_thread(mc::BufferQueue &bundle, int nframes)
{
   bool enable_frame_dropping{false};
   int const nframes_to_test_before_switching{5};
   for (int i = 0; i < nframes; ++i)
   {
       bundle.allow_framedropping(enable_frame_dropping);
       for (int j = 0; j < nframes_to_test_before_switching; j++)
           bundle.client_release(client_acquire_sync(bundle));
       enable_frame_dropping = !enable_frame_dropping;
       std::this_thread::yield();
   }
}
}

TEST_F(BufferQueueWithOneBuffer, buffer_queue_of_one_is_supported)
{
    auto handle = client_acquire_async(q);

    /* Client is allowed to get the only buffer in existence */
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    /* Client blocks until the client releases
     * the buffer and compositor composites it*/
    auto next_request = client_acquire_async(q);
    EXPECT_THAT(next_request->has_acquired_buffer(), Eq(false));

    auto comp_buffer = q.compositor_acquire(this);
    auto client_id = handle->id();

    /* Client and compositor always share the same buffer */
    EXPECT_THAT(client_id, Eq(comp_buffer->id()));

    EXPECT_NO_THROW(handle->release_buffer());
    EXPECT_NO_THROW(q.compositor_release(comp_buffer));

    /* Simulate a composite pass */
    comp_buffer = q.compositor_acquire(this);
    q.compositor_release(comp_buffer);

    /* The request should now be fullfilled after compositor
     * released the buffer
     */
    EXPECT_THAT(next_request->has_acquired_buffer(), Eq(true));
    EXPECT_NO_THROW(next_request->release_buffer());
}

TEST_F(BufferQueueWithOneBuffer, buffer_queue_of_one_supports_resizing)
{
    const geom::Size expect_size{10, 20};
    q.resize(expect_size);

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    auto buffer = handle->buffer();
    ASSERT_THAT(buffer->size(), Eq(expect_size));

    /* Client and compositor share the same buffer so
     * expect the new size
     */
    std::shared_ptr<mg::Buffer> comp_buffer;
    ASSERT_NO_THROW(comp_buffer = q.compositor_acquire(this));

    EXPECT_THAT(buffer->size(), Eq(expect_size));
    EXPECT_NO_THROW(q.compositor_release(comp_buffer));

    EXPECT_NO_THROW(handle->release_buffer());
    EXPECT_NO_THROW(q.compositor_release(q.compositor_acquire(this)));
}

TEST_F(BufferQueueWithTwoBuffers, framedropping_is_disabled_by_default)
{
    EXPECT_THAT(q.framedropping_allowed(), Eq(false));
}

TEST_F(BufferQueue, throws_when_creating_with_invalid_num_buffers)
{
    EXPECT_THROW(mc::BufferQueue a(0, allocator, basic_properties, policy_factory), std::logic_error);
    EXPECT_THROW(mc::BufferQueue a(-1, allocator, basic_properties, policy_factory), std::logic_error);
    EXPECT_THROW(mc::BufferQueue a(-10, allocator, basic_properties, policy_factory), std::logic_error);
}

TEST_P(WithAnyNumberOfBuffers, client_can_acquire_and_release_buffer)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    ASSERT_NO_THROW(handle->release_buffer());
}

TEST_P(WithTwoOrMoreBuffers, client_can_acquire_buffers)
{
    int const max_ownable_buffers = q.buffers_free_for_client();
    ASSERT_THAT(max_ownable_buffers, Gt(0));

    for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    }
}

/* Regression test for LP: #1315302 */
TEST_F(BufferQueueWithThreeBuffers, clients_can_have_multiple_pending_completions)
{
    int const prefill = q.buffers_free_for_client();
    ASSERT_THAT(prefill, Gt(0));
    for (int i = 0; i < prefill; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
    }

    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(false));

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(false));

    for (int i = 0; i < nbuffers + 1; ++i)
        q.compositor_release(q.compositor_acquire(this));

    EXPECT_THAT(handle1->has_acquired_buffer(), Eq(true));
    EXPECT_THAT(handle2->has_acquired_buffer(), Eq(true));
    EXPECT_THAT(handle1->buffer(), Ne(handle2->buffer()));

    handle1->release_buffer();
    handle2->release_buffer();
}

TEST_P(WithTwoOrMoreBuffers, compositor_acquires_frames_in_order_for_synchronous_client)
{
    ASSERT_THAT(q.framedropping_allowed(), Eq(false));

    void const* main_compositor = reinterpret_cast<void const*>(0);
    void const* second_compositor = reinterpret_cast<void const*>(1);
    for (int i = 0; i < 20; i++)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        auto client_id = handle->id();
        handle->release_buffer();

        auto comp_buffer = q.compositor_acquire(main_compositor);
        auto composited_id = comp_buffer->id();
        q.compositor_release(comp_buffer);

        EXPECT_THAT(composited_id, Eq(client_id));

        comp_buffer = q.compositor_acquire(second_compositor);
        EXPECT_THAT(composited_id, Eq(comp_buffer->id()));
        q.compositor_release(comp_buffer);
    }
}

TEST_P(WithTwoOrMoreBuffers, framedropping_clients_never_block)
{
    q.allow_framedropping(true);

    for (int i = 0; i < 1000; i++)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
    }
}

/* Regression test for LP: #1210042 */
TEST_F(BufferQueueWithThreeBuffers, clients_dont_recycle_startup_buffer)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    auto client_id = handle->id();
    handle->release_buffer();

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    auto comp_buffer = q.compositor_acquire(this);
    EXPECT_THAT(client_id, Eq(comp_buffer->id()));
    q.compositor_release(comp_buffer);
}

TEST_P(WithThreeOrMoreBuffers, throws_on_out_of_order_client_release)
{
    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(true));

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle2->has_acquired_buffer(), Eq(true));

    EXPECT_THROW(handle2->release_buffer(), std::logic_error);
    EXPECT_NO_THROW(handle1->release_buffer());

    EXPECT_THROW(handle1->release_buffer(), std::logic_error);
    EXPECT_NO_THROW(handle2->release_buffer());
}

TEST_P(WithTwoOrMoreBuffers, async_client_cycles_through_all_buffers)
{
    // This test is technically not valid with dynamic queue scaling on
    q.set_scaling_delay(-1);

    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };
    mt::AutoUnblockThread compositor(unblock,
        unthrottled_compositor_thread, std::ref(q), std::ref(done));

    std::unordered_set<uint32_t> ids_acquired;
    int const max_ownable_buffers = nbuffers - 1;
    for (int i = 0; i < max_ownable_buffers*2; ++i)
    {
        std::vector<mg::Buffer *> client_buffers;
        for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            ids_acquired.insert(handle->id().as_value());
            client_buffers.push_back(handle->buffer());
        }

        for (auto const& buffer : client_buffers)
        {
            q.client_release(buffer);
        }
    }

    EXPECT_THAT(ids_acquired.size(), Eq(nbuffers));
}

TEST_P(WithAnyNumberOfBuffers, compositor_can_acquire_and_release)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    auto client_id = handle->id();
    ASSERT_NO_THROW(handle->release_buffer());

    auto comp_buffer = q.compositor_acquire(this);
    EXPECT_THAT(client_id, Eq(comp_buffer->id()));
    EXPECT_NO_THROW(q.compositor_release(comp_buffer));
}

TEST_P(WithTwoOrMoreBuffers, clients_get_new_buffers_on_compositor_release)
{   // Regression test for LP: #1480164
    q.allow_framedropping(false);

    // Skip over the first frame. The early release optimization is too
    // conservative to allow it to happen right at the start (so as to
    // maintain correct multimonitor frame rates if required).
    auto handle = client_acquire_async(q);
    ASSERT_TRUE(handle->has_acquired_buffer());
    handle->release_buffer();
    q.compositor_release(q.compositor_acquire(this));

    auto onscreen = q.compositor_acquire(this);

    // This is what tests should do instead of using buffers_free_for_client()
    bool blocking;
    do
    {
        handle = client_acquire_async(q);
        blocking = !handle->has_acquired_buffer();
        if (!blocking)
            handle->release_buffer();
    } while (!blocking);

    for (int f = 0; f < 100; ++f)
    {
        ASSERT_FALSE(handle->has_acquired_buffer());
        q.compositor_release(onscreen);
        ASSERT_TRUE(handle->has_acquired_buffer()) << "frame# " << f;
        handle->release_buffer();
        onscreen = q.compositor_acquire(this);
        handle = client_acquire_async(q);
    }
}

TEST_P(WithTwoOrMoreBuffers, short_buffer_holds_dont_overclock_multimonitor)
{   // Regression test related to LP: #1480164
    q.allow_framedropping(false);

    // Skip over the first frame. The early release optimization is too
    // conservative to allow it to happen right at the start (so as to
    // maintain correct multimonitor frame rates if required).
    auto handle = client_acquire_async(q);
    ASSERT_TRUE(handle->has_acquired_buffer());
    handle->release_buffer();

    const void* const leftid = "left";
    const void* const rightid = "right";
    auto left = q.compositor_acquire(leftid);
    q.compositor_release(left);
    left = q.compositor_acquire(leftid);
    auto right = q.compositor_acquire(rightid);

    // This is what tests should do instead of using buffers_free_for_client()
    bool blocking;
    do
    {
        handle = client_acquire_async(q);
        blocking = !handle->has_acquired_buffer();
        if (!blocking)
            handle->release_buffer();
    } while (!blocking);

    for (int f = 0; f < 100; ++f)
    {
        ASSERT_FALSE(handle->has_acquired_buffer());
        q.compositor_release(left);
        q.compositor_release(right);
        ASSERT_FALSE(handle->has_acquired_buffer());
        left = q.compositor_acquire(leftid);
        right = q.compositor_acquire(rightid);
        ASSERT_TRUE(handle->has_acquired_buffer());
        handle->release_buffer();
        handle = client_acquire_async(q);
    }
}

TEST_P(WithThreeOrMoreBuffers, greedy_clients_get_new_buffers_on_compositor_release)
{   // Regression test for LP: #1480164
    q.allow_framedropping(false);

    // Skip over the first frame. The early release optimization is too
    // conservative to allow it to happen right at the start (so as to
    // maintain correct multimonitor frame rates if required).
    auto handle = client_acquire_async(q);
    ASSERT_TRUE(handle->has_acquired_buffer());
    handle->release_buffer();
    q.compositor_release(q.compositor_acquire(this));

    auto onscreen = q.compositor_acquire(this);
    auto old_handle = handle;
    old_handle.reset();
    bool blocking;
    do
    {
        handle = client_acquire_async(q);
        blocking = !handle->has_acquired_buffer();
        if (!blocking)
        {
            if (old_handle)
                old_handle->release_buffer();
            old_handle = handle;
            handle.reset();
        }
    } while (!blocking);

    ASSERT_TRUE(old_handle->has_acquired_buffer());
    ASSERT_FALSE(handle->has_acquired_buffer());
    q.compositor_release(onscreen);
    ASSERT_TRUE(handle->has_acquired_buffer());
}

TEST_P(WithAnyNumberOfBuffers, multiple_compositors_are_in_sync)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    auto client_id = handle->id();
    ASSERT_NO_THROW(handle->release_buffer());

    for (int monitor = 0; monitor < 10; monitor++)
    {
        void const* user_id = reinterpret_cast<void const*>(monitor);
        auto comp_buffer = q.compositor_acquire(user_id);
        EXPECT_THAT(client_id, Eq(comp_buffer->id()));
        q.compositor_release(comp_buffer);
    }
}

TEST_P(WithThreeOrMoreBuffers, multiple_fast_compositors_are_in_sync)
{  // Regression test for LP: #1420678
    // Client generates first frame
    auto handle1 = client_acquire_async(q);
    ASSERT_TRUE(handle1->has_acquired_buffer());
    auto client_id1 = handle1->id();
    handle1->release_buffer();

    // Client generates second frame
    auto handle2 = client_acquire_async(q);
    ASSERT_TRUE(handle2->has_acquired_buffer());
    auto client_id2 = handle2->id();
    handle2->release_buffer();

    // Many monitors... verify they all get the first frame.
    for (int monitor = 0; monitor < 10; monitor++)
    {
        void const* user_id = reinterpret_cast<void const*>(monitor);
        auto comp_buffer = q.compositor_acquire(user_id);
        ASSERT_EQ(client_id1, comp_buffer->id());
        q.compositor_release(comp_buffer);
    }

    // Still many monitors... verify they all get the second frame.
    for (int monitor = 0; monitor < 10; monitor++)
    {
        void const* user_id = reinterpret_cast<void const*>(monitor);
        auto comp_buffer = q.compositor_acquire(user_id);
        ASSERT_EQ(client_id2, comp_buffer->id());
        q.compositor_release(comp_buffer);
    }
}

TEST_P(WithTwoOrMoreBuffers, compositor_acquires_frames_in_order)
{
    for (int i = 0; i < 10; ++i)
    {
        std::deque<mg::BufferID> client_release_sequence;
        std::vector<mg::Buffer *> buffers;
        for (int i = 0; i < q.buffers_free_for_client(); ++i)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            buffers.push_back(handle->buffer());
        }

        for (auto buffer : buffers)
        {
            client_release_sequence.push_back(buffer->id());
            q.client_release(buffer);
        }

        for (auto const& client_id : client_release_sequence)
        {
            auto comp_buffer = q.compositor_acquire(this);
            EXPECT_THAT(client_id, Eq(comp_buffer->id()));
            q.compositor_release(comp_buffer);
        }
    }
}

TEST_P(WithAnyNumberOfBuffers, compositor_acquire_never_blocks_when_there_are_no_ready_buffers)
{
    for (int i = 0; i < 100; i++)
    {
        auto buffer = q.compositor_acquire(this);
        q.compositor_release(buffer);
    }
}

TEST_P(WithTwoOrMoreBuffers, compositor_can_always_acquire_buffer)
{
    q.allow_framedropping(false);

    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };

    mt::AutoJoinThread client([this]
    {
        for (int nframes = 0; nframes < 100; ++nframes)
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            handle->release_buffer();
            std::this_thread::yield();
        }
    });
    mt::AutoUnblockThread compositor(unblock, [&]
    {
        while (!done)
        {
            std::shared_ptr<mg::Buffer> buffer;
            EXPECT_NO_THROW(buffer = q.compositor_acquire(this));
            EXPECT_THAT(buffer, Ne(nullptr));
            EXPECT_NO_THROW(q.compositor_release(buffer));
            std::this_thread::yield();
        }
    });

    client.stop();
    compositor.stop();
}

TEST_P(WithAnyNumberOfBuffers, compositor_acquire_recycles_latest_ready_buffer)
{
    mg::Buffer* last_client_acquired_buffer{nullptr};

    for (int i = 0; i < 20; i++)
    {
        if (i % 10 == 0)
        {
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            last_client_acquired_buffer = handle->buffer();
            handle->release_buffer();
        }

        for (int monitor_id = 0; monitor_id < 10; monitor_id++)
        {
            void const* user_id = reinterpret_cast<void const*>(monitor_id);
            auto buffer = q.compositor_acquire(user_id);
            ASSERT_THAT(buffer.get(), Eq(last_client_acquired_buffer));
            q.compositor_release(buffer);
        }
    }
}

TEST_P(WithAnyNumberOfBuffers, compositor_release_verifies_parameter)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    auto comp_buffer = q.compositor_acquire(this);
    q.compositor_release(comp_buffer);
    EXPECT_THROW(q.compositor_release(comp_buffer), std::logic_error);
}

/* Regression test for LP#1270964 */
TEST_F(BufferQueueWithThreeBuffers, compositor_client_interleaved)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    auto first_ready_buffer_id = handle->id();
    handle->release_buffer();

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

    // in the original bug, compositor would be given the wrong buffer here
    auto compositor_buffer = q.compositor_acquire(this);

    EXPECT_THAT(compositor_buffer->id(), Eq(first_ready_buffer_id));

    handle->release_buffer();
    q.compositor_release(compositor_buffer);
}

TEST_P(WithTwoOrMoreBuffers, overlapping_compositors_get_different_frames)
{
    // This test simulates bypass behaviour
    std::shared_ptr<mg::Buffer> compositor[2];

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();
    compositor[0] = q.compositor_acquire(this);

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();
    compositor[1] = q.compositor_acquire(this);

    for (int i = 0; i < 20; i++)
    {
        // Two compositors acquired, and they're always different...
        ASSERT_THAT(compositor[0]->id(), Ne(compositor[1]->id()));

        // One of the compositors (the oldest one) gets a new buffer...
        int oldest = i & 1;
        q.compositor_release(compositor[oldest]);
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        compositor[oldest] = q.compositor_acquire(this);
    }

    q.compositor_release(compositor[0]);
    q.compositor_release(compositor[1]);
}

TEST_P(WithAnyNumberOfBuffers, snapshot_acquire_basic)
{
    auto comp_buffer = q.compositor_acquire(this);
    auto snapshot = q.snapshot_acquire();
    EXPECT_THAT(snapshot->id(), Eq(comp_buffer->id()));
    q.compositor_release(comp_buffer);
    q.snapshot_release(snapshot);
}

TEST_F(BufferQueueWithOneBuffer, callbacks_cant_happen_after_shutdown)
{
    q.drop_client_requests();
    auto client = client_acquire_async(q);
    ASSERT_FALSE(client->has_acquired_buffer());
}

TEST_F(BufferQueueWithOneBuffer, callbacks_cant_happen_after_shutdown_with_snapshots)
{
    auto snapshot = q.snapshot_acquire();
    q.drop_client_requests();
    q.snapshot_release(snapshot);

    auto client = client_acquire_async(q);
    ASSERT_FALSE(client->has_acquired_buffer());
}

TEST_P(WithAnyNumberOfBuffers, snapshot_acquire_never_blocks)
{
    int const num_snapshots = 100;

    std::shared_ptr<mg::Buffer> buf[num_snapshots];
    for (int i = 0; i < num_snapshots; i++)
        buf[i] = q.snapshot_acquire();

    for (int i = 0; i < num_snapshots; i++)
        q.snapshot_release(buf[i]);
}

TEST_P(WithTwoOrMoreBuffers, snapshot_release_verifies_parameter)
{
    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    auto comp_buffer = q.compositor_acquire(this);
    EXPECT_THROW(q.snapshot_release(comp_buffer), std::logic_error);

    handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    auto snapshot = q.snapshot_acquire();

    EXPECT_THAT(snapshot->id(), Eq(comp_buffer->id()));
    EXPECT_THAT(snapshot->id(), Ne(handle->id()));
    EXPECT_NO_THROW(q.snapshot_release(snapshot));
    EXPECT_THROW(q.snapshot_release(snapshot), std::logic_error);
}

TEST_P(WithTwoOrMoreBuffers, stress)
{
    std::atomic<bool> done(false);

    auto unblock = [&done]{ done = true;};

    mt::AutoUnblockThread compositor(unblock, unthrottled_compositor_thread,
                                     std::ref(q),
                                     std::ref(done));
    mt::AutoUnblockThread snapshotter1(unblock, snapshot_thread,
                                       std::ref(q),
                                       std::ref(done));
    mt::AutoUnblockThread snapshotter2(unblock, snapshot_thread,
                                       std::ref(q),
                                       std::ref(done));

    q.allow_framedropping(false);
    mt::AutoJoinThread client1(client_thread, std::ref(q), 1000);
    client1.stop();

    q.allow_framedropping(true);
    mt::AutoJoinThread client2(client_thread, std::ref(q), 1000);
    client2.stop();

    mt::AutoJoinThread client3(switching_client_thread, std::ref(q), 1000);
    client3.stop();
}

TEST_P(WithTwoOrMoreBuffers, framedropping_clients_get_all_buffers)
{
    q.allow_framedropping(true);

    int const nframes = 100;
    std::unordered_set<uint32_t> ids_acquired;
    for (int i = 0; i < nframes; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        ids_acquired.insert(handle->id().as_value());
        handle->release_buffer();
    }

    EXPECT_THAT(ids_acquired.size(), Ge(nbuffers));
}

TEST_P(WithTwoOrMoreBuffers, waiting_clients_unblock_on_shutdown)
{
    q.allow_framedropping(false);

    int const max_ownable_buffers = q.buffers_free_for_client();
    ASSERT_THAT(max_ownable_buffers, Gt(0));

    for (int b = 0; b < max_ownable_buffers; b++)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
    }

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(false));

    q.force_requests_to_complete();

    EXPECT_THAT(handle->has_acquired_buffer(), Eq(true));
}

TEST_P(WithTwoOrMoreBuffers, client_framerate_matches_compositor)
{
    unsigned long client_frames = 0;
    const unsigned long compose_frames = 20;

    q.allow_framedropping(false);

    std::atomic<bool> done(false);

    mt::AutoJoinThread monitor1([&]
    {
        for (unsigned long frame = 0; frame != compose_frames+3; frame++)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(10));

            auto buf = q.compositor_acquire(this);
            q.compositor_release(buf);

            if (frame == compose_frames)
            {
                // Tell the "client" to stop after compose_frames, but
                // don't stop rendering immediately to avoid blocking
                // if we rendered any twice
                done.store(true);
            }
        }
    });

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    while (!done.load())
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
        client_frames++;
    }

    monitor1.stop();

    // Roughly compose_frames == client_frames within 50%
    ASSERT_THAT(client_frames, Gt(compose_frames / 2));
    ASSERT_THAT(client_frames, Lt(compose_frames * 3 / 2));
}

/* Regression test LP: #1241369 / LP: #1241371 */
TEST_P(WithThreeOrMoreBuffers, slow_client_framerate_matches_compositor)
{
    /* BufferQueue can only satify this for nbuffers >= 3
     * since a client can only own up to nbuffers - 1 at any one time
     */
    unsigned long client_frames = 0;
    unsigned long const compose_frames = 100;
    auto const frame_time = std::chrono::milliseconds(16);

    q.allow_framedropping(false);

    std::atomic<bool> done(false);
    std::mutex sync;

    mt::AutoJoinThread monitor1([&]
    {
        for (unsigned long frame = 0; frame != compose_frames+3; frame++)
        {
            std::this_thread::sleep_for(frame_time);
            sync.lock();
            auto buf = q.compositor_acquire(this);
            q.compositor_release(buf);
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

    auto handle = client_acquire_async(q);
    ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
    handle->release_buffer();

    while (!done.load())
    {
        sync.lock();
        sync.unlock();
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        std::this_thread::sleep_for(frame_time);
        handle->release_buffer();
        client_frames++;
    }

    monitor1.stop();

    // Roughly compose_frames == client_frames within 20%
    ASSERT_THAT(client_frames, Gt(compose_frames * 0.80f));
    ASSERT_THAT(client_frames, Lt(compose_frames * 1.2f));
}

TEST_P(WithAnyNumberOfBuffers, resize_affects_client_acquires_immediately)
{
    for (int width = 1; width < 100; ++width)
    {
        const geom::Size expect_size{width, width * 2};

        for (int subframe = 0; subframe < 3; ++subframe)
        {
            q.resize(expect_size);
            auto handle = client_acquire_async(q);
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            auto buffer = handle->buffer();
            ASSERT_THAT(expect_size, Eq(buffer->size()));
            handle->release_buffer();

            auto comp_buffer = q.compositor_acquire(this);
            ASSERT_THAT(expect_size, Eq(comp_buffer->size()));
            q.compositor_release(comp_buffer);
        }
    }
}

namespace
{
int max_ownable_buffers(int nbuffers)
{
    return (nbuffers == 1) ? 1 : nbuffers - 1;
}
}

TEST_P(WithAnyNumberOfBuffers, compositor_acquires_resized_frames)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);
    std::vector<mg::BufferID> history;

    const int width0 = 123;
    const int height0 = 456;
    const int dx = 2;
    const int dy = -3;
    int width = width0;
    int height = height0;
    int const nbuffers_to_use = q.buffers_free_for_client();
    ASSERT_THAT(nbuffers_to_use, Gt(0));

    int max_buffers{max_ownable_buffers(nbuffers)};
    for (int produce = 0; produce < max_buffers; ++produce)
    {
        geom::Size new_size{width, height};
        width += dx;
        height += dy;

        q.resize(new_size);
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        history.emplace_back(handle->id());
        auto buffer = handle->buffer();
        ASSERT_THAT(buffer->size(), Eq(new_size));
        handle->release_buffer();
    }

    width = width0;
    height = height0;

    ASSERT_THAT(history.size(), Eq(max_buffers));
    for (int consume = 0; consume < max_buffers; ++consume)
    {
        geom::Size expect_size{width, height};
        width += dx;
        height += dy;

        auto buffer = q.compositor_acquire(this);

        // Verify the compositor gets resized buffers, eventually
        ASSERT_THAT(buffer->size(), Eq(expect_size));

        // Verify the compositor gets buffers with *contents*, ie. that
        // they have not been resized prematurely and are empty.
        ASSERT_THAT(history[consume], Eq(buffer->id()));

        q.compositor_release(buffer);
    }

    // Verify the final buffer size sticks
    const geom::Size final_size{width - dx, height - dy};
    for (int unchanging = 0; unchanging < 100; ++unchanging)
    {
        auto buffer = q.compositor_acquire(this);
        ASSERT_THAT(buffer->size(), Eq(final_size));
        q.compositor_release(buffer);
    }
}

TEST_P(WithTwoOrMoreBuffers, framedropping_policy_never_drops_newest_frame)
{  // Regression test for LP: #1396006
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    auto first = client_acquire_sync(q);
    q.client_release(first);

    // Start rendering one (don't finish)
    auto d = q.compositor_acquire(nullptr);
    ASSERT_EQ(first, d.get());

    auto second = client_acquire_sync(q);
    q.client_release(second);

    // Client waits for a new frame
    auto end = client_acquire_async(q);

    // Surface goes offscreen or occluded; trigger a timeout
    policy_factory.trigger_policies();

    // If the queue is still willing to drop under these difficult
    // circumstances (and we don't mind if it doesn't), then ensure
    // it's never the newest frame that's been discarded.
    // That could be catastrophic as you never know if a client ever
    // will produce another frame.
    if (end->has_acquired_buffer())
        ASSERT_NE(second, end->buffer());

    q.compositor_release(d);
}

TEST_P(WithTwoOrMoreBuffers, framedropping_surface_never_drops_newest_frame)
{  // Second regression test for LP: #1396006, LP: #1379685
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    q.allow_framedropping(true);

    // Fill 'er up
    std::vector<mg::Buffer*> order;
    for (int f = 0; f < nbuffers; ++f)
    {
        auto b = client_acquire_sync(q);
        order.push_back(b);
        q.client_release(b);
    }

    // Composite all but one
    std::vector<std::shared_ptr<mg::Buffer>> compositing;
    for (int n = 0; n < nbuffers-1; ++n)
    {
        auto c = q.compositor_acquire(nullptr);
        compositing.push_back(c);
        ASSERT_EQ(order[n], c.get());
    }

    // Ensure it's not the newest frame that gets dropped to satisfy the
    // client.
    auto end = client_acquire_async(q);

    // The queue could solve this problem a few ways. It might choose to
    // defer framedropping till it's safe, or even allocate additional
    // buffers. We don't care which, just verify it's not losing the
    // latest frame. Because the screen could be indefinitely out of date
    // if that happens...
    ASSERT_TRUE(!end->has_acquired_buffer() ||
                end->buffer() != order.back());
}

TEST_P(WithTwoOrMoreBuffers, uncomposited_client_swaps_when_policy_triggered)
{
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    for (int i = 0; i < max_ownable_buffers(nbuffers); i++)
    {
        auto client = client_acquire_sync(q);
        q.client_release(client);
    }

    auto handle = client_acquire_async(q);

    EXPECT_FALSE(handle->has_acquired_buffer());

    policy_factory.trigger_policies();
    EXPECT_TRUE(handle->has_acquired_buffer());
}

TEST_P(WithTwoOrMoreBuffers, scaled_queue_still_follows_dropping_policy)
{   // Regression test for LP: #1475120
    using namespace std::literals::chrono_literals;

    auto clock = std::make_shared<mt::FakeClock>();
    auto constexpr framedrop_timeout = 10ms;

    mc::TimeoutFrameDroppingPolicyFactory policy_factory{
        std::make_shared<mtd::FakeTimer>(clock),
        framedrop_timeout};

    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);
    
    int const nframes = 100;

    q.set_scaling_delay(0);
    for (int i = 0; i < nframes; i++)
    {
        auto handle = client_acquire_async(q);

        // Advance time past the framedrop timeout.
        clock->advance_time(framedrop_timeout);
        clock->advance_time(1ms);

        // If we fail once we don't need to run the rest of the 100 iterations...
        ASSERT_TRUE(handle->has_acquired_buffer());
        handle->release_buffer();
    }
}

TEST_P(WithTwoOrMoreBuffers, partially_composited_client_swaps_when_policy_triggered)
{
    mtd::MockFrameDroppingPolicyFactory policy_factory;
    mc::BufferQueue q(nbuffers,
                      allocator,
                      basic_properties,
                      policy_factory);

    for (int i = 0; i < max_ownable_buffers(nbuffers); i++)
    {
        auto client = client_acquire_sync(q);
        q.client_release(client);
    }

    /* Queue up two pending swaps */
    auto first_swap = client_acquire_async(q);
    auto second_swap = client_acquire_async(q);

    ASSERT_FALSE(first_swap->has_acquired_buffer());
    ASSERT_FALSE(second_swap->has_acquired_buffer());

    q.compositor_acquire(nullptr);

    EXPECT_TRUE(first_swap->has_acquired_buffer());
    EXPECT_FALSE(second_swap->has_acquired_buffer());

    /* We have to release a client buffer here; framedropping or not,
     * a client can't have 2 buffers outstanding in the nbuffers = 2 case.
     */
    first_swap->release_buffer();

    policy_factory.trigger_policies();
    EXPECT_TRUE(second_swap->has_acquired_buffer());
}

TEST_F(BufferQueueWithOneBuffer, with_single_buffer_compositor_acquires_resized_frames_eventually)
{
    geom::Size const new_size{123,456};

    q.client_release(client_acquire_sync(q));
    q.resize(new_size);

    auto const handle = client_acquire_async(q);
    EXPECT_THAT(handle->has_acquired_buffer(), Eq(false));

    auto buf = q.compositor_acquire(this);
    q.compositor_release(buf);

    buf = q.compositor_acquire(this);
    EXPECT_THAT(buf->size(), Eq(new_size));
    q.compositor_release(buf);
}

TEST_F(BufferQueueWithTwoBuffers, double_buffered_client_is_not_blocked_prematurely)
{  // Regression test for LP: #1319765
    q.client_release(client_acquire_sync(q));
    auto a = q.compositor_acquire(this);
    q.client_release(client_acquire_sync(q));
    auto b = q.compositor_acquire(this);

    ASSERT_NE(a.get(), b.get());

    q.compositor_release(a);
    q.client_release(client_acquire_sync(q));

    q.compositor_release(b);

    /*
     * Update to the original test case; This additional compositor acquire
     * represents the fixing of LP: #1395581 in the compositor logic.
     */
    if (q.buffers_ready_for_compositor(this))
        q.compositor_release(q.compositor_acquire(this));

    auto handle = client_acquire_async(q);
    // With the fix, a buffer will be available instantaneously:
    ASSERT_TRUE(handle->has_acquired_buffer());
    handle->release_buffer();
}

TEST_F(BufferQueueWithTwoBuffers, composite_on_demand_never_deadlocks_with_2_buffers)
{  // Extended regression test for LP: #1319765
    for (int i = 0; i < 100; ++i)
    {
        auto x = client_acquire_async(q);
        ASSERT_TRUE(x->has_acquired_buffer());
        x->release_buffer();

        auto a = q.compositor_acquire(this);

        auto y = client_acquire_async(q);
        ASSERT_TRUE(y->has_acquired_buffer());
        y->release_buffer();

        auto b = q.compositor_acquire(this);
    
        ASSERT_NE(a.get(), b.get());
    
        q.compositor_release(a);

        auto w = client_acquire_async(q);
        ASSERT_TRUE(w->has_acquired_buffer());
        w->release_buffer();
    
        q.compositor_release(b);

        /*
         * Update to the original test case; This additional compositor acquire
         * represents the fixing of LP: #1395581 in the compositor logic.
         */
        if (q.buffers_ready_for_compositor(this))
            q.compositor_release(q.compositor_acquire(this));

        auto z = client_acquire_async(q);
        ASSERT_TRUE(z->has_acquired_buffer());
        z->release_buffer();

        q.compositor_release(q.compositor_acquire(this));
        q.compositor_release(q.compositor_acquire(this));
    }
}

TEST_P(WithTwoOrMoreBuffers, buffers_ready_is_not_underestimated)
{  // Regression test for LP: #1395581
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};

    // Produce frame 1
    q.client_release(client_acquire_sync(q));
    // Acquire frame 1
    auto a = q.compositor_acquire(this);

    // Produce frame 2
    q.client_release(client_acquire_sync(q));
    // Acquire frame 2
    auto b = q.compositor_acquire(this);

    // Release frame 1
    q.compositor_release(a);
    // Produce frame 3
    q.client_release(client_acquire_sync(q));
    // Release frame 2
    q.compositor_release(b);

    // Verify frame 3 is ready for the first compositor
    ASSERT_THAT(q.buffers_ready_for_compositor(this), Ge(1));
    auto c = q.compositor_acquire(this);

    // Verify frame 3 is ready for a second compositor
    int const that = 0;
    ASSERT_THAT(q.buffers_ready_for_compositor(&that), Ge(1));

    q.compositor_release(c);
}

TEST_P(WithTwoOrMoreBuffers, buffers_ready_eventually_reaches_zero)
{
    mc::BufferQueue q{nbuffers, allocator, basic_properties, policy_factory};

    const int nmonitors = 3;
    int monitor[nmonitors];

    for (int m = 0; m < nmonitors; ++m)
    {
        ASSERT_EQ(0, q.buffers_ready_for_compositor(&monitor[m]));
    }

    // Produce a frame
    q.client_release(client_acquire_sync(q));

    // Consume
    for (int m = 0; m < nmonitors; ++m)
    {
        ASSERT_NE(0, q.buffers_ready_for_compositor(&monitor[m]));

        // Double consume to account for the +1 that
        // buffers_ready_for_compositor adds to do dynamic performance
        // detection.
        q.compositor_release(q.compositor_acquire(&monitor[m]));
        q.compositor_release(q.compositor_acquire(&monitor[m]));

        ASSERT_EQ(0, q.buffers_ready_for_compositor(&monitor[m]));
    }
}

/* Regression test for LP: #1306464 */
TEST_F(BufferQueueWithThreeBuffers, framedropping_client_acquire_does_not_block_when_no_available_buffers)
{
    q.allow_framedropping(true);

    std::vector<std::shared_ptr<mg::Buffer>> buffers;

    /* The client can never own this acquired buffer */
    auto comp_buffer = q.compositor_acquire(this);
    buffers.push_back(comp_buffer);

    /* Let client release all possible buffers so they go into
     * the ready queue
     */
    for (int i = 0; i < nbuffers; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        /* Check the client never got the compositor buffer acquired above */
        ASSERT_THAT(handle->id(), Ne(comp_buffer->id()));
        handle->release_buffer();
    }

    /* Let the compositor acquire all ready buffers */
    for (int i = 0; i < nbuffers; ++i)
    {
        buffers.push_back(q.compositor_acquire(this));
    }

    /* At this point the queue has 0 free buffers and 0 ready buffers
     * so the next client request should not be satisfied until
     * a compositor releases its buffers */
    auto handle = client_acquire_async(q);
    /* ... unless the BufferQueue is overallocating. In that case it will
     * have succeeding in acquiring immediately.
     */ 
    if (!handle->has_acquired_buffer())
    {
        /* Release compositor buffers so that the client can get one */
        for (auto const& buffer : buffers)
        {
            q.compositor_release(buffer);
        }
        EXPECT_THAT(handle->has_acquired_buffer(), Eq(true));
    }
}

TEST_P(WithTwoOrMoreBuffers, compositor_never_owns_client_buffers)
{
    static std::chrono::nanoseconds const time_for_client_to_acquire{1};

    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);

    std::mutex client_buffer_guard;
    std::condition_variable client_buffer_cv;
    mg::Buffer* client_buffer = nullptr;
    std::atomic<bool> done(false);

    auto unblock = [&done]{ done = true; };
    mt::AutoUnblockThread unthrottled_compositor_thread(unblock, [&]
    {
        while (!done)
        {
            auto buffer = q.compositor_acquire(this);

            {
                std::unique_lock<std::mutex> lock(client_buffer_guard);

                if (client_buffer_cv.wait_for(
                    lock,
                    time_for_client_to_acquire,
                    [&]()->bool{ return client_buffer; }))
                {
                    ASSERT_THAT(buffer->id(), Ne(client_buffer->id()));
                }
            }

            std::this_thread::yield();
            q.compositor_release(buffer);
        }
    });

    for (int i = 0; i < 1000; ++i)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        {
            std::lock_guard<std::mutex> lock(client_buffer_guard);
            client_buffer = handle->buffer();
            client_buffer_cv.notify_one();
        }

        std::this_thread::yield();

        std::lock_guard<std::mutex> lock(client_buffer_guard);
        handle->release_buffer();
        client_buffer = nullptr;
    }
}

TEST_P(WithTwoOrMoreBuffers, client_never_owns_compositor_buffers)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);
    for (int i = 0; i < 100; ++i)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        auto client_id = handle->id();
        std::vector<std::shared_ptr<mg::Buffer>> buffers;
        for (int j = 0; j < nbuffers; j++)
        {
            auto buffer = q.compositor_acquire(this);
            ASSERT_THAT(client_id, Ne(buffer->id()));
            buffers.push_back(buffer);
        }

        for (auto const& buffer: buffers)
            q.compositor_release(buffer);

        handle->release_buffer();

        /* Flush out one ready buffer */
        auto buffer = q.compositor_acquire(this);
        ASSERT_THAT(client_id, Eq(buffer->id()));
        q.compositor_release(buffer);
    }
}

/* Regression test for an issue brought up at:
 * http://code.launchpad.net/~albaguirre/mir/
 * alternative-switching-bundle-implementation/+merge/216606/comments/517048
 */
TEST_P(WithThreeOrMoreBuffers, buffers_are_not_lost)
{
    // This test is technically not valid with dynamic queue scaling on
    q.set_scaling_delay(-1);

    void const* main_compositor = reinterpret_cast<void const*>(0);
    void const* second_compositor = reinterpret_cast<void const*>(1);

    /* Hold a reference to current compositor buffer*/
    auto comp_buffer1 = q.compositor_acquire(main_compositor);

    int const prefill = q.buffers_free_for_client();
    ASSERT_THAT(prefill, Gt(0));
    for (int acquires = 0; acquires < prefill; ++acquires)
    {
        auto handle = client_acquire_async(q);
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
        handle->release_buffer();
    }

    /* Have a second compositor advance the current compositor buffer at least twice */
    for (int acquires = 0; acquires < nbuffers; ++acquires)
    {
        auto comp_buffer = q.compositor_acquire(second_compositor);
        q.compositor_release(comp_buffer);
    }
    q.compositor_release(comp_buffer1);

    /* An async client should still be able to cycle through all the available buffers */
    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };
    mt::AutoUnblockThread compositor(unblock,
       unthrottled_compositor_thread, std::ref(q), std::ref(done));

    std::unordered_set<mg::Buffer *> unique_buffers_acquired;
    int const max_ownable_buffers = nbuffers - 1;
    for (int frame = 0; frame < max_ownable_buffers*2; frame++)
    {
        std::vector<mg::Buffer *> client_buffers;
        for (int acquires = 0; acquires < max_ownable_buffers; ++acquires)
        {
            auto handle = client_acquire_async(q);
            handle->wait_for(std::chrono::seconds(1));
            ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));
            unique_buffers_acquired.insert(handle->buffer());
            client_buffers.push_back(handle->buffer());
        }

        for (auto const& buffer : client_buffers)
        {
            q.client_release(buffer);
        }
    }

    EXPECT_THAT(unique_buffers_acquired.size(), Eq(nbuffers));
}

// Test that dynamic queue scaling/throttling actually works
TEST_P(WithThreeOrMoreBuffers, queue_size_scales_with_client_performance)
{
    q.allow_framedropping(false);
    std::unordered_set<mg::Buffer *> buffers_acquired;

    int const delay = 3;
    q.set_scaling_delay(delay);

    int const nframes = 100;

    for (int frame = 0; frame < nframes;)
    {
        std::shared_ptr<AcquireWaitHandle> client;
        do
        {
            client = client_acquire_async(q);
            if (client->has_acquired_buffer())
            {
                if (frame > delay)
                    buffers_acquired.insert(client->buffer());
                client->release_buffer();
                client.reset();
            }
        } while (!client);

        while (q.buffers_ready_for_compositor(nullptr))
        {
            q.compositor_release(q.compositor_acquire(nullptr));
            ++frame;
        }

        if (client->has_acquired_buffer())
        {
            if (frame > delay)
                buffers_acquired.insert(client->buffer());
            client->release_buffer();
            client.reset();
        }
    }
    // Expect double-buffers for fast clients
    EXPECT_THAT(buffers_acquired.size(), Eq(2));

    // Now check what happens if the client becomes slow...
    buffers_acquired.clear();
    for (int frame = 0; frame < nframes;)
    {
        std::shared_ptr<AcquireWaitHandle> client;
        do
        {
            client = client_acquire_async(q);
            if (client->has_acquired_buffer())
            {
                if (frame > delay)
                    buffers_acquired.insert(client->buffer());
                client->release_buffer();
                client.reset();
            }
        } while (!client);

        while (q.buffers_ready_for_compositor(nullptr))
        {
            q.compositor_release(q.compositor_acquire(nullptr));
            ++frame;
        }

        if (client->has_acquired_buffer())
        {
            if (frame > delay)
                buffers_acquired.insert(client->buffer());
            client->release_buffer();
            client.reset();
        }

        // Balance compositor consumption with client production:
        while (q.buffers_ready_for_compositor(nullptr))
        {
            q.compositor_release(q.compositor_acquire(nullptr));
            ++frame;
        }

        // Imbalance: Compositor is now requesting more than the client does:
        q.compositor_release(q.compositor_acquire(nullptr));
        ++frame;
    }
    // Expect at least triple buffers for sluggish clients
    EXPECT_THAT(buffers_acquired.size(), Ge(3));

    // And what happens if the client becomes fast again?...
    buffers_acquired.clear();
    for (int frame = 0; frame < nframes;)
    {
        std::shared_ptr<AcquireWaitHandle> client;
        do
        {
            client = client_acquire_async(q);
            if (client->has_acquired_buffer())
            {
                if (frame > delay)
                    buffers_acquired.insert(client->buffer());
                client->release_buffer();
                client.reset();
            }
        } while (!client);

        while (q.buffers_ready_for_compositor(nullptr))
        {
            q.compositor_release(q.compositor_acquire(nullptr));
            ++frame;
        }

        if (client->has_acquired_buffer())
        {
            if (frame > delay)
                buffers_acquired.insert(client->buffer());
            client->release_buffer();
            client.reset();
        }
    }
    // Expect double-buffers for fast clients
    EXPECT_THAT(buffers_acquired.size(), Eq(2));
}

TEST_P(WithThreeOrMoreBuffers, greedy_compositors_need_triple_buffers)
{
    /*
     * "Greedy" compositors means those that can hold multiple buffers from
     * the same client simultaneously or a single buffer for a long time.
     * This usually means bypass/overlays, but can also mean multi-monitor.
     */
    q.allow_framedropping(false);

    std::atomic<bool> done(false);
    auto unblock = [&done] { done = true; };

    mt::AutoUnblockThread compositor(unblock,
       overlapping_compositor_thread, std::ref(q), std::ref(done));

    std::unordered_set<mg::Buffer *> buffers_acquired;
    int const delay = q.scaling_delay();

    for (int frame = 0; frame < 10; frame++)
    {
        auto handle = client_acquire_async(q);
        handle->wait_for(std::chrono::seconds(1));
        ASSERT_THAT(handle->has_acquired_buffer(), Eq(true));

        if (frame > delay)
            buffers_acquired.insert(handle->buffer());
        handle->release_buffer();
    }
    // Expect triple buffers for the whole time
    EXPECT_THAT(buffers_acquired.size(), Ge(3));
}

TEST_P(WithTwoOrMoreBuffers, compositor_double_rate_of_slow_client)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);
    q.allow_framedropping(false);

    q.set_scaling_delay(3);

    for (int frame = 0; frame < 10; frame++)
    {
        ASSERT_EQ(0, q.buffers_ready_for_compositor(this));
        q.client_release(client_acquire_sync(q));

        // Detecting a slow client requires scheduling at least one extra
        // frame...
        int nready = q.buffers_ready_for_compositor(this);
        ASSERT_THAT(nready, Ge(2));
        for (int i = 0; i < nready; ++i)
            q.compositor_release(q.compositor_acquire(this));
        ASSERT_EQ(0, q.buffers_ready_for_compositor(this));
    }
}

/*
 * This is a regression test for bug lp:1317801. This bug is a race and
 * very difficult to reproduce with pristine code. By carefully placing
 * a delay in the code, we can greatly increase the chances (100% for me)
 * that this test catches a regression. However these delays are not
 * acceptable for production use, so since the test and code in their
 * pristine state are highly unlikely to catch the issue, I have decided
 * to DISABLE the test to avoid giving a false sense of security.
 *
 * Apply the aforementioned delay, by adding
 * std::this_thread::sleep_for(std::chrono::milliseconds{20})
 * just before returning the acquired_buffer at the end of
 * BufferQueue::compositor_acquire().
 */
TEST_F(BufferQueueWithThreeBuffers, DISABLED_lp_1317801_regression_test)
{
    q.client_release(client_acquire_sync(q));

    mt::AutoJoinThread t{
        [&]
        {
            /* Use in conjuction with a 20ms delay in compositor_acquire() */
            std::this_thread::sleep_for(std::chrono::milliseconds{10});

            q.client_release(client_acquire_sync(q));
            q.client_release(client_acquire_sync(q));
        }};

    auto b = q.compositor_acquire(this);
    q.compositor_release(b);
}

TEST_P(WithAnyNumberOfBuffers, first_user_is_recorded)
{
    mc::BufferQueue q(nbuffers, allocator, basic_properties, policy_factory);

    auto comp = q.compositor_acquire(this);
    EXPECT_TRUE(q.is_a_current_buffer_user(this));
    q.compositor_release(comp);
}

TEST_F(BufferQueueWithThreeBuffers, gives_compositor_a_valid_buffer_after_dropping_old_buffers_without_clients)
{
    q.drop_old_buffers();

    auto comp = q.compositor_acquire(this);
    ASSERT_THAT(comp, Ne(nullptr));
}

TEST_F(BufferQueueWithThreeBuffers, gives_compositor_the_newest_buffer_after_dropping_old_buffers)
{
    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(true));
    handle1->release_buffer();

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle2->has_acquired_buffer(), Eq(true));
    handle2->release_buffer();

    q.drop_old_buffers();

    auto comp = q.compositor_acquire(this);
    ASSERT_THAT(comp->id(), Eq(handle2->id()));
    q.compositor_release(comp);

    comp = q.compositor_acquire(this);
    ASSERT_THAT(comp->id(), Eq(handle2->id()));
}

TEST_F(BufferQueueWithThreeBuffers, gives_new_compositor_the_newest_buffer_after_dropping_old_buffers)
{
    void const* const new_compositor_id{&nbuffers};

    auto handle1 = client_acquire_async(q);
    ASSERT_THAT(handle1->has_acquired_buffer(), Eq(true));
    handle1->release_buffer();

    auto comp = q.compositor_acquire(this);
    ASSERT_THAT(comp->id(), Eq(handle1->id()));
    q.compositor_release(comp);

    auto handle2 = client_acquire_async(q);
    ASSERT_THAT(handle2->has_acquired_buffer(), Eq(true));
    handle2->release_buffer();

    q.drop_old_buffers();

    auto comp2 = q.compositor_acquire(new_compositor_id);
    ASSERT_THAT(comp2->id(), Eq(handle2->id()));
}

INSTANTIATE_TEST_CASE_P(
    BufferQueue,
    WithAnyNumberOfBuffers,
    Range(1, max_nbuffers_to_test));
INSTANTIATE_TEST_CASE_P(
    BufferQueue,
    WithTwoOrMoreBuffers,
    Range(2, max_nbuffers_to_test));
INSTANTIATE_TEST_CASE_P(
    BufferQueue,
    WithThreeOrMoreBuffers,
    Range(3, max_nbuffers_to_test));
