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

#include "mir_test_framework/testing_server_configuration.h"

#include "mir/graphics/display.h"
#include "mir/graphics/platform.h"
#include "mir/graphics/platform_ipc_package.h"
#include "mir/graphics/renderer.h"
#include "mir/graphics/renderable.h"
#include "mir/compositor/buffer_basic.h"
#include "mir/compositor/buffer_properties.h"
#include "mir/compositor/buffer_ipc_package.h"
#include "mir/compositor/graphic_buffer_allocator.h"
#include "mir/input/input_channel.h"
#include "mir/input/input_manager.h"

#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_surface_builder.h"
#include "mir_test_doubles/null_display.h"

#include <gtest/gtest.h>
#include <thread>

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mf = mir::frontend;
namespace mtf = mir_test_framework;
namespace mtd = mir::test::doubles;

namespace
{
char const* dummy[] = {0};
int argc = 0;
char const** argv = dummy;

geom::Rectangle const default_view_area = geom::Rectangle{geom::Point(),
                                                                 geom::Size{geom::Width(1600),
                                                                            geom::Height(1600)}};

class StubGraphicBufferAllocator : public mc::GraphicBufferAllocator
{
 public:
    std::shared_ptr<mc::Buffer> alloc_buffer(mc::BufferProperties const& properties)
    {
        return std::unique_ptr<mc::Buffer>(new mtd::StubBuffer(properties));
    }

    std::vector<geom::PixelFormat> supported_pixel_formats()
    {
        return std::vector<geom::PixelFormat>();
    }
};

class StubDisplay : public mg::Display
{
public:
    geom::Rectangle view_area() const
    {
        return default_view_area;
    }
    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f)
    {
        (void)f;
        std::this_thread::yield();
    }
    std::shared_ptr<mg::DisplayConfiguration> configuration()
    {
        auto null_configuration = std::shared_ptr<mg::DisplayConfiguration>();
        return null_configuration;
    }
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
    virtual void render(std::function<void(std::shared_ptr<void> const&)>, mg::Renderable& r)
    {
        // Need to acquire the texture to cycle buffers
        r.graphic_region();
    }
};

struct StubInputChannel : public mi::InputChannel
{
    int client_fd() const
    {
        return 0;
    }

    int server_fd() const
    {
        return 0;
    }
};

class StubInputManager : public mi::InputManager
{
  public:
    void start() {}
    void stop() {}

    std::shared_ptr<mi::InputChannel> make_input_channel()
    {
        return std::make_shared<StubInputChannel>();
    }

    void set_input_focus_to(std::shared_ptr<mi::SessionTarget> const& /* session */, std::shared_ptr<mi::SurfaceTarget> const& /* surface */)
    {
    }
};
}

mtf::TestingServerConfiguration::TestingServerConfiguration() :
    DefaultServerConfiguration(argc, argv)
{
}


std::shared_ptr<mi::InputManager> mtf::TestingServerConfiguration::the_input_manager()
{
    auto options = the_options();
    if (options->get("tests-use-real-input", false))
        return mi::create_input_manager(the_event_filters(), the_display());
    else
        return std::make_shared<StubInputManager>();
}

std::shared_ptr<mg::Platform> mtf::TestingServerConfiguration::the_graphics_platform()
{
    if (!graphics_platform)
    {
        graphics_platform = std::make_shared<StubGraphicPlatform>();
    }

    return graphics_platform;
}

std::shared_ptr<mg::Renderer> mtf::TestingServerConfiguration::the_renderer()
{
    auto options = the_options();

    if (options->get("tests-use-real-graphics", false))
        return DefaultServerConfiguration::the_renderer();
    else
        return renderer(
            [&]()
            {
                return std::make_shared<StubRenderer>();
            });
}

void mtf::TestingServerConfiguration::exec(DisplayServer* )
{
}

void mtf::TestingServerConfiguration::on_exit()
{
}

std::string mtf::TestingServerConfiguration::the_socket_file() const
{
    return test_socket_file();
}


std::string const& mtf::test_socket_file()
{
    static const std::string socket_file{"./mir_socket_test"};
    return socket_file;
}


int main(int argc, char** argv)
{
    ::argc = std::remove_if(
        argv,
        argv+argc,
        [](char const* arg) { return !strncmp(arg, "--gtest_", 8); }) - argv;
    ::argv = const_cast<char const**>(argv);

  ::testing::InitGoogleTest(&argc, argv);

  return RUN_ALL_TESTS();
}
