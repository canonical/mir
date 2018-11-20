/*
 * Copyright © 2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 or 3 as
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "vsync_simulating_graphics_platform.h"

#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/test/doubles/null_platform_ipc_operations.h"

#include <chrono>
#include <functional>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace mtd = mir::test::doubles;

namespace
{

struct StubDisplaySyncGroup : mg::DisplaySyncGroup
{
    StubDisplaySyncGroup(geom::Size output_size, int vsync_rate_in_hz) :
        vsync_rate_in_hz(vsync_rate_in_hz),
        last_sync(std::chrono::high_resolution_clock::now()),
        buffer({{0, 0}, output_size})
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& exec) override
    {
        exec(buffer);
    }

    void post() override
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto next_sync = last_sync + std::chrono::seconds(1) / vsync_rate_in_hz;
        
        if (now < next_sync)
            std::this_thread::sleep_for(next_sync - now);
        
        last_sync = now;
    }

    std::chrono::milliseconds recommended_sleep() const override
    {
        return std::chrono::milliseconds::zero();
    }
    
    double const vsync_rate_in_hz;

    std::chrono::high_resolution_clock::time_point last_sync;

    mtd::StubDisplayBuffer buffer;
};

struct StubDisplay : public mtd::StubDisplay
{
    StubDisplay(geom::Size output_size, int vsync_rate_in_hz) :
        mtd::StubDisplay({{{0,0}, output_size}}),
        group(output_size, vsync_rate_in_hz)
    {
    }
    
    void for_each_display_sync_group(std::function<void(mg::DisplaySyncGroup&)> const& exec) override
    {
        exec(group);
    }

    StubDisplaySyncGroup group;
};

}

VsyncSimulatingPlatform::VsyncSimulatingPlatform(geom::Size const& output_size, int vsync_rate_in_hz)
    : output_size(output_size), vsync_rate_in_hz(vsync_rate_in_hz)
{
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> VsyncSimulatingPlatform::create_buffer_allocator(
    mg::Display const&)
{
    return mir::make_module_ptr<mtd::StubBufferAllocator>();
}

mir::UniqueModulePtr<mg::Display> VsyncSimulatingPlatform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
     std::shared_ptr<mg::GLConfig> const&)
{
    return mir::make_module_ptr<StubDisplay>(output_size, vsync_rate_in_hz);
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> VsyncSimulatingPlatform::make_ipc_operations() const
{
    return mir::make_module_ptr<mtd::NullPlatformIpcOperations>();
}
