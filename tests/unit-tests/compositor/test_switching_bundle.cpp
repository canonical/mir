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

#include "src/server/compositor/switching_bundle.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_buffer.h"

#include <gtest/gtest.h>

namespace geom=mir::geometry;
namespace mtd=mir::test::doubles;
namespace mc=mir::compositor;

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

TEST_F(SwitchingBundleTest, client_acquire_with_switch)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);

    auto buffer = bundle.client_acquire();
    bundle.allow_framedropping(true);
    bundle.client_release(buffer); 
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

TEST_F(SwitchingBundleTest, compositor_acquire_with_switch)
{
    mc::SwitchingBundle bundle(2, allocator, basic_properties);

    auto client = bundle.client_acquire();
    auto client_id = client->id();
    bundle.client_release(client);

    auto compositor = bundle.compositor_acquire();
    EXPECT_EQ(client_id, compositor->id());
    bundle.allow_framedropping(true);
    bundle.compositor_release(compositor); 
}

