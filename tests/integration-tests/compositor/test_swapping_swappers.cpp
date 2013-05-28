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
        auto z1 = new mtd::StubBuffer;
        auto z2 = new mtd::StubBuffer;
        auto z3 = new mtd::StubBuffer;
        buffer_a = std::shared_ptr<mtd::StubBuffer>(z1, [](mtd::StubBuffer* b){printf("dela\n"); delete b;});
        buffer_b = std::shared_ptr<mtd::StubBuffer>(z2, [](mtd::StubBuffer* b){printf("delb\n"); delete b;});
        buffer_c = std::shared_ptr<mtd::StubBuffer>(z3, [](mtd::StubBuffer* b){printf("delc\n"); delete b;});
        auto triple_list = std::initializer_list<std::shared_ptr<mc::Buffer>>{buffer_a, buffer_b, buffer_c};
        auto first_swapper = std::make_shared<mc::BufferSwapperMulti>(triple_list);
        swapper_switcher = std::make_shared<mc::SwapperSwitcher>(first_swapper);
    }

    std::shared_ptr<mc::Buffer> buffer_a, buffer_b, buffer_c;
    std::shared_ptr<mc::SwapperSwitcher> swapper_switcher;
};

TEST_F(SwapperSwappingStress, swapper)
{
    auto f = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 400; i++)
                    {
                        auto b = swapper_switcher->client_acquire();
                        std::this_thread::yield();
                        swapper_switcher->client_release(b);
                    }
                });

    auto g = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 10000; i++)
                    {
                        auto b = swapper_switcher->compositor_acquire();
                        std::this_thread::yield();
                        swapper_switcher->compositor_release(b);
                    } 
                });

    auto h = std::async(std::launch::async,
                [this]
                {
                    for(auto i=0u; i < 100000; i++)
                    {
                        auto list = std::initializer_list<std::shared_ptr<mc::Buffer>>{};
                        auto new_swapper = std::make_shared<mc::BufferSwapperMulti>(list);
                        swapper_switcher->switch_swapper(new_swapper); 
                    } 
                });

    f.wait();
    g.wait();
    h.wait();
}
