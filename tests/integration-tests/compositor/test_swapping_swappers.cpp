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

#include "mir_test_doubles/stub_buffer_allocator.h"
#include "multithread_harness.h"

#include "src/server/compositor/switching_bundle.h"
#include "src/server/compositor/buffer_stream_surfaces.h"
#include "mir/graphics/graphic_buffer_allocator.h"

#include <gmock/gmock.h>
#include <future>
#include <thread>
#include <chrono>

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
                                          geom::PixelFormat::abgr_8888,
                                          mg::BufferUsage::hardware};
        switching_bundle = std::make_shared<mc::SwitchingBundle>(3, allocator, properties);
    }

    std::shared_ptr<mc::SwitchingBundle> switching_bundle;
    std::atomic<bool> client_thread_done;
};
}

TEST_F(SwapperSwappingStress, swapper)
{
    client_thread_done = false;

    auto f = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 400; i++)
                    {
                        auto b = switching_bundle->client_acquire();
                        std::this_thread::yield();
                        switching_bundle->client_release(b);
                    }
                    client_thread_done = true;
                });

    auto g = std::async(std::launch::async,
                [this]
                {
                    unsigned long count = 0;
                    while(!client_thread_done)
                    {
                        auto b = switching_bundle->compositor_acquire(++count);
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
    client_thread_done = false;

    auto f = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 400; i++)
                    {
                        auto b = switching_bundle->client_acquire();
                        std::this_thread::yield();
                        switching_bundle->client_release(b);
                    }
                    client_thread_done = true;
                });

    auto g = std::async(std::launch::async,
                [this]
                {
                    unsigned long count = 0;
                    while(!client_thread_done)
                    {
                        auto b = switching_bundle->compositor_acquire(++count);
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
