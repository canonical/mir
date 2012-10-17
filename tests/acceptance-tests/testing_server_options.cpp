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

#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/renderer.h"
#include "mir/compositor/buffer.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/graphic_buffer_allocator.h"

#include <boost/program_options/config.hpp>
#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include <cstdlib>

#include <fstream>

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

namespace
{
// Load config from ~/.ubuntu_display_server.config
// 1. The only config currently supported is "stub_graphics_in_tests".
// 2. This is a temporary home for this functionality, but it suffices for now.
boost::program_options::variables_map get_config()
{
    namespace po = boost::program_options;
    po::variables_map options;
    try
    {
        std::string filename;
        if (const char* home = getenv("HOME")) (filename += home) += "/";
        filename += ".ubuntu_display_server.config";

        std::ifstream file(filename);
        po::options_description desc("Options");
        desc.add_options()
            ("stub_graphics_in_tests", po::value<bool>(), "stub graphics in tests");

        po::store(po::parse_config_file(file, desc), options, true);
        po::notify(options);
    }
    catch (const po::error& error)
    {
        std::cerr << "ERROR: " << error.what() << std::endl;
    }
    return options;
}
}

std::shared_ptr<mg::Renderer> mir::TestingServerConfiguration::make_renderer(
        std::shared_ptr<mg::Display> const& display)
{
    auto options = get_config();

    bool stub_graphics{true};

    if (options.count("stub_graphics_in_tests"))
    {
        stub_graphics = options["stub_graphics_in_tests"].as<bool>();
    }

    if (stub_graphics)
        return std::make_shared<StubRenderer>();
    else
        return DefaultServerConfiguration::make_renderer(display);
}

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

