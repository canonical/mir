/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "testing_process_manager.h"

/////////////////////////////////////////////////////////////////////
// ===>> TODO start of frig to stub out graphics from tests <<=== //
// We stub out the graphics - because real graphics cause all the
// tests to fail. There must be a better way?

#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/renderer.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/graphic_buffer_allocator.h"

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;

namespace
{
class StubBuffer : public mc::Buffer
{
    geom::Size size() const { return geom::Size(); }

    geom::Stride stride() const { return geom::Stride(); }

    geom::PixelFormat pixel_format() const { return geom::PixelFormat(); }

    std::shared_ptr<mc::BufferIPCPackage> get_ipc_package() const { return std::make_shared<mc::BufferIPCPackage>(); }

    void bind_to_texture() {}

};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    std::unique_ptr<mc::Buffer> alloc_buffer(geom::Size, geom::PixelFormat)
    {
        return std::unique_ptr<mc::Buffer>(new StubBuffer());
    }
};

class StubDisplay : public mg::Display
{
 public:
    geom::Rectangle view_area() const { return geom::Rectangle(); }
    void clear() {}
    bool post_update() { return true; }
};

class StubGraphicPlatform : public mg::Platform
{
    virtual std::shared_ptr<mc::GraphicBufferAllocator> create_buffer_allocator(
            const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/)
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    virtual std::shared_ptr<mg::Display> create_display()
    {
        return std::make_shared<StubDisplay>();
    }

    virtual std::shared_ptr<mg::PlatformIPCPackage> get_ipc_package()
    {
        return std::make_shared<mg::PlatformIPCPackage>();
    }
};

class StubRenderer : public mg::Renderer
{
public:
    virtual void render(mg::Renderable&)
    {
    }
};
}

std::shared_ptr<mg::Platform> mir::TestingServerConfiguration::make_graphics_platform()
{
    if (!graphics_platform)
    {
        graphics_platform = std::make_shared<StubGraphicPlatform>();
    }

    return graphics_platform;
}

std::shared_ptr<mg::Renderer> mir::TestingServerConfiguration::make_renderer(
        std::shared_ptr<mg::Display> const& /*display*/)
{
    return std::make_shared<StubRenderer>();
}

// ====>> TODO end of frig to stub out graphics from tests <<==== //
/////////////////////////////////////////////////////////////////////

void mir::TestingServerConfiguration::exec(DisplayServer* )
{
}

void mir::TestingServerConfiguration::on_exit(DisplayServer* )
{
}

mir::TestingServerConfiguration::TestingServerConfiguration() :
    DefaultServerConfiguration(test_socket_file())
{
}

