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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_frame_dropping_policy_factory.h"
#include "multithread_harness.h"

#include "src/server/compositor/buffer_queue.h"
#include "src/server/compositor/buffer_stream_surfaces.h"
#include "mir/graphics/graphic_buffer_allocator.h"

#include <gmock/gmock.h>
#include <future>
#include <thread>
#include <chrono>
#include <mutex>
#include <atomic>

namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mt = mir::testing;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

namespace
{

struct SwapperSwappingStress : public ::testing::Test
{
    void SetUp()
    {
        auto allocator = std::make_shared<mtd::StubBufferAllocator>();
        auto properties = mg::BufferProperties{geom::Size{380, 210},
                                          mir_pixel_format_abgr_8888,
                                          mg::BufferUsage::hardware};
        mtd::StubFrameDroppingPolicyFactory policy_factory;
        switching_bundle = std::make_shared<mc::BufferQueue>(3, allocator, properties, policy_factory);
    }

    std::shared_ptr<mc::BufferQueue> switching_bundle;
    std::mutex acquire_mutex;  // must live longer than our callback/lambda

    mg::Buffer* client_acquire_blocking(
        std::shared_ptr<mc::BufferQueue> const& switching_bundle)
    {
        std::condition_variable cv;
        bool acquired = false;
    
        mg::Buffer* result;
        switching_bundle->client_acquire(
            [&](mg::Buffer* new_buffer)
             {
                std::unique_lock<decltype(acquire_mutex)> lock(acquire_mutex);
    
                result = new_buffer;
                acquired = true;
                cv.notify_one();
             });
    
        std::unique_lock<decltype(acquire_mutex)> lock(acquire_mutex);
    
        cv.wait(lock, [&]{ return acquired; });
    
        return result;
    }
};

} // namespace

TEST_F(SwapperSwappingStress, swapper)
{
    std::atomic_bool done(false);

    auto f = std::async(std::launch::async,
                [&]
                {
                    for(auto i=0u; i < 400; i++)
                    {
                        auto b = client_acquire_blocking(switching_bundle);
                        std::this_thread::yield();
                        switching_bundle->client_release(b);
                    }
                    done = true;
                });

    auto g = std::async(std::launch::async,
                [&]
                {
                    while (!done)
                    {
                        auto b = switching_bundle->compositor_acquire(0);
                        std::this_thread::yield();
                        switching_bundle->compositor_release(b);
                    }
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 100; i++)
                    {
                        switching_bundle->allow_framedropping(true);
                        std::this_thread::yield();
                        switching_bundle->allow_framedropping(false);
                        std::this_thread::yield();
                    }
                });

    f.wait();
    g.wait();
    h.wait();
}

TEST_F(SwapperSwappingStress, different_swapper_types)
{
    std::atomic_bool done(false);

    auto f = std::async(std::launch::async,
                [&]
                {
                    for(auto i=0u; i < 400; i++)
                    {
                        auto b = client_acquire_blocking(switching_bundle);
                        std::this_thread::yield();
                        switching_bundle->client_release(b);
                    }
                    done = true;
                });

    auto g = std::async(std::launch::async,
                [&]
                {
                    while (!done)
                    {
                        auto b = switching_bundle->compositor_acquire(0);
                        std::this_thread::yield();
                        switching_bundle->compositor_release(b);
                    }
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 200; i++)
                    {
                        switching_bundle->allow_framedropping(true);
                        std::this_thread::yield();
                        switching_bundle->allow_framedropping(false);
                        std::this_thread::yield();
                    }
                });

    f.wait();
    g.wait();
    h.wait();
}
