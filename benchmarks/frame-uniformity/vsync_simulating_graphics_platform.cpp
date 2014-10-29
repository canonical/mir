/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored by: Robert Carr <robert.carr@canonical.com>
 */

#include "vsync_simulating_graphics_platform.h"

#include "mir/graphics/buffer_writer.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_ipc_package.h"

#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_display.h"

#include <chrono>
#include <functional>

namespace mg = mir::graphics;
namespace geom = mir::geometry;

namespace mtd = mir::test::doubles;

namespace
{

struct StubBufferWriter : public mg::BufferWriter
{
    void write(mg::Buffer&, unsigned char const*, size_t) override
    {
    }
};

class StubIpcOps : public mg::PlatformIpcOperations
{
    void pack_buffer(
        mg::BufferIpcMessage&,
        mg::Buffer const&,
        mg::BufferIpcMsgType) const override
    {
    }

    void unpack_buffer(
        mg::BufferIpcMessage&, mg::Buffer const&) const override
    {
    }

    std::shared_ptr<mg::PlatformIPCPackage> connection_ipc_package() override
    {
        return std::make_shared<mg::PlatformIPCPackage>();
    }
};

struct StubDisplayBuffer : mtd::StubDisplayBuffer
{
    StubDisplayBuffer(geom::Size output_size, int vsync_rate_in_hz)
        : mtd::StubDisplayBuffer({{0, 0}, output_size}),
          vsync_rate_in_hz(vsync_rate_in_hz),
          last_sync(std::chrono::high_resolution_clock::now())
    {
    }
    
    void post_update() override
    {
        auto now = std::chrono::high_resolution_clock::now();
        auto next_sync = last_sync + std::chrono::seconds(1) / vsync_rate_in_hz;
        
        if (now < next_sync)
            std::this_thread::sleep_for(next_sync - now);
        
        last_sync = now;
    }
    
    double const vsync_rate_in_hz;

    std::chrono::high_resolution_clock::time_point last_sync;
};

struct StubDisplay : public mtd::StubDisplay
{
    StubDisplay(geom::Size output_size, int vsync_rate_in_hz)
        : mtd::StubDisplay({{{0,0}, output_size}}),
          buffer(output_size, vsync_rate_in_hz)
    {
    }
    
    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& exec) override
    {
        exec(buffer);
    }

    StubDisplayBuffer buffer;
};

}

VsyncSimulatingPlatform::VsyncSimulatingPlatform(geom::Size const& output_size, int vsync_rate_in_hz)
    : display(std::make_shared<StubDisplay>(output_size, vsync_rate_in_hz))
{
}

std::shared_ptr<mg::GraphicBufferAllocator> VsyncSimulatingPlatform::create_buffer_allocator()
{
    return std::make_shared<mtd::StubBufferAllocator>();
}

std::shared_ptr<mg::BufferWriter> VsyncSimulatingPlatform::make_buffer_writer()
{
    return std::make_shared<StubBufferWriter>();
}
    
std::shared_ptr<mg::Display> VsyncSimulatingPlatform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
    std::shared_ptr<mg::GLProgramFactory> const&,
     std::shared_ptr<mg::GLConfig> const&)
{
    return display;
}
    
std::shared_ptr<mg::PlatformIpcOperations> VsyncSimulatingPlatform::make_ipc_operations() const
{
    return std::make_shared<StubIpcOps>();
}

std::shared_ptr<mg::InternalClient> VsyncSimulatingPlatform::create_internal_client()
{
    return nullptr;
}
