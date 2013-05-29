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

#include "mir_test_doubles/stub_buffer.h"
#include "multithread_harness.h"

#include "src/server/compositor/swapper_switcher.h"
#include "mir/compositor/buffer_swapper_multi.h"
#include "mir/compositor/buffer_swapper_spin.h"
#include "mir/compositor/buffer_bundle_surfaces.h"

#include <future>
#include <thread>
#include <chrono>

namespace mc = mir::compositor;
namespace mt = mir::testing;
namespace geom = mir::geometry;
namespace mtd = mir::test::doubles;

struct SwapperSwappingStress : public ::testing::Test
{
    void SetUp()
    {
        buffer_a = std::shared_ptr<mtd::StubBuffer>(std::make_shared<mtd::StubBuffer>());
        buffer_b = std::shared_ptr<mtd::StubBuffer>(std::make_shared<mtd::StubBuffer>());
        buffer_c = std::shared_ptr<mtd::StubBuffer>(std::make_shared<mtd::StubBuffer>());
        auto triple_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b, buffer_c};
        auto first_swapper = std::make_shared<mc::BufferSwapperMulti>(triple_list);
        swapper_switcher = std::make_shared<mc::SwapperSwitcher>(first_swapper);
    }

    std::shared_ptr<mc::Buffer> buffer_a, buffer_b, buffer_c;
    std::shared_ptr<mc::SwapperSwitcher> swapper_switcher;
    std::atomic<bool> client_thread_done;
};

TEST_F(SwapperSwappingStress, swapper)
{
    client_thread_done = false;

    auto f = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 400; i++)
                    {
                        auto b = swapper_switcher->client_acquire();
                        std::this_thread::yield();
                        swapper_switcher->client_release(b);
                    }
                    client_thread_done = true;
                });

    auto g = std::async(std::launch::async,
                [this]
                {
                    while(!client_thread_done)
                    {
                        auto b = swapper_switcher->compositor_acquire();
                        std::this_thread::yield();
                        swapper_switcher->compositor_release(b);
                    }
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 200; i++)
                    {
#if 0
                        auto list = std::initializer_list<std::shared_ptr<mc::Buffer>>{};
                        auto new_swapper = std::make_shared<mc::BufferSwapperMulti>(list);
                        swapper_switcher->transfer_buffers_to(new_swapper); 
                        std::this_thread::yield();
#endif
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
                        auto b = swapper_switcher->client_acquire();
                        std::this_thread::yield();
                        swapper_switcher->client_release(b);
                    }
                    client_thread_done = true;
                });

    auto g = std::async(std::launch::async,
                [this]
                {
                    while(!client_thread_done)
                    {
                        auto b = swapper_switcher->compositor_acquire();
                        std::this_thread::yield();
                        swapper_switcher->compositor_release(b);
                    }
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 200; i++)
                    {
#if 0
                        auto list = std::initializer_list<std::shared_ptr<mc::Buffer>>{};
                        auto new_swapper = std::make_shared<mc::BufferSwapperMulti>(list);
                        swapper_switcher->transfer_buffers_to(new_swapper); 
                        std::this_thread::yield();
                        auto new_async_swapper = std::make_shared<mc::BufferSwapperSpin>(list);
                        swapper_switcher->transfer_buffers_to(new_async_swapper);
#endif 
                    } 
                });

    f.wait();
    g.wait();
    h.wait();
}
