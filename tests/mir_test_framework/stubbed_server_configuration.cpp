/*
 * Copyright © 2013-2014 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
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

#include "mir_test_framework/stubbed_server_configuration.h"

#include "mir/options/default_configuration.h"
#include "mir/geometry/rectangle.h"
#include "mir/graphics/buffer_ipc_packer.h"
#include "mir/input/input_channel.h"
#include "mir/input/input_manager.h"

#include "mir_test_doubles/null_display.h"
#include "mir_test_doubles/null_display_configuration.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_renderer.h"

#ifdef ANDROID
#include "mir_test_doubles/mock_android_native_buffer.h"
#endif

#include "mir/compositor/renderer.h"
#include "mir/compositor/renderer_factory.h"
#include "src/server/input/null_input_configuration.h"

#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mo = mir::options;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
char const* dummy[] = {0};
int argc = 0;
char const** argv = dummy;

class StubFDBuffer : public mtd::StubBuffer
{
public:
    StubFDBuffer(mg::BufferProperties const& properties)
        : StubBuffer(properties),
          properties{properties}
    {
        fd = open("/dev/zero", O_RDONLY);
        if (fd < 0)
            BOOST_THROW_EXCEPTION(
                boost::enable_error_info(
                    std::runtime_error("Failed to open dummy fd")) << boost::errinfo_errno(errno));
    }

    std::shared_ptr<mg::NativeBuffer> native_buffer_handle() const override
    {
#ifndef ANDROID
        auto native_buffer = std::make_shared<mg::NativeBuffer>();
        native_buffer->data_items = 1;
        native_buffer->data[0] = 0xDEADBEEF;
        native_buffer->fd_items = 1;
        native_buffer->fd[0] = fd;
        native_buffer->width = properties.size.width.as_int();
        native_buffer->height = properties.size.height.as_int();

        native_buffer->flags = 0;
        if (properties.size.width.as_int() >= 800 &&
            properties.size.height.as_int() >= 600 &&
            properties.usage == mg::BufferUsage::hardware)
        {
            native_buffer->flags |= mir_buffer_flag_can_scanout;
        }
#else
        auto native_buffer = std::make_shared<mtd::StubAndroidNativeBuffer>();
        auto anwb = native_buffer->anwb();
        anwb->width = properties.size.width.as_int();
        anwb->height = properties.size.width.as_int();
#endif
        return native_buffer;
    }

    ~StubFDBuffer() noexcept
    {
        close(fd);
    }
private:
    int fd;
    const mg::BufferProperties properties;
};

class StubGraphicBufferAllocator : public mtd::StubBufferAllocator
{
 public:
    std::shared_ptr<mg::Buffer> alloc_buffer(mg::BufferProperties const& properties) override
    {
        return std::make_shared<StubFDBuffer>(properties);
    }
};

class StubDisplayConfiguration : public mtd::NullDisplayConfiguration
{
public:
    StubDisplayConfiguration(geom::Rectangle const& rect)
         : modes{mg::DisplayConfigurationMode{rect.size, 1.0f}}
    {
    }

    void for_each_output(std::function<void(mg::DisplayConfigurationOutput const&)> f) const override
    {
        mg::DisplayConfigurationOutput dummy_output_config{
            mg::DisplayConfigurationOutputId{1},
            mg::DisplayConfigurationCardId{0},
            mg::DisplayConfigurationOutputType::vga,
            std::vector<MirPixelFormat>{mir_pixel_format_abgr_8888},
            modes, 0, geom::Size{}, true, true, geom::Point{0,0}, 0,
            mir_pixel_format_abgr_8888, mir_power_mode_on,
            mir_orientation_normal};

        f(dummy_output_config);
    }
private:
    std::vector<mg::DisplayConfigurationMode> modes;
};

class StubDisplay : public mtd::NullDisplay
{
public:
    StubDisplay()
        : rect{geom::Point{0,0}, geom::Size{1600,1600}},
          display_buffer(rect)
    {
    }

    void for_each_display_buffer(std::function<void(mg::DisplayBuffer&)> const& f) override
    {
        f(display_buffer);
    }

    std::unique_ptr<mg::DisplayConfiguration> configuration() const override
    {
        return std::unique_ptr<mg::DisplayConfiguration>(
            new StubDisplayConfiguration(rect)
        );
    }

private:
    geom::Rectangle rect;
    mtd::StubDisplayBuffer display_buffer;
};

class StubGraphicPlatform : public mtd::NullPlatform
{
    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/) override
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    void fill_ipc_package(mg::BufferIPCPacker* packer, mg::Buffer const* buffer) const override
    {
#ifndef ANDROID
        auto native_handle = buffer->native_buffer_handle();
        for(auto i=0; i<native_handle->data_items; i++)
        {
            packer->pack_data(native_handle->data[i]);
        }
        for(auto i=0; i<native_handle->fd_items; i++)
        {
            packer->pack_fd(native_handle->fd[i]);
        }

        packer->pack_flags(native_handle->flags);
#endif
        packer->pack_stride(buffer->stride());
        packer->pack_size(buffer->size());
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&) override
    {
        return std::make_shared<StubDisplay>();
    }
};

class StubRendererFactory : public mc::RendererFactory
{
public:
    std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&)
    {
        return std::unique_ptr<mc::Renderer>(new mtd::StubRenderer());
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
};
}

mtf::StubbedServerConfiguration::StubbedServerConfiguration() :
    DefaultServerConfiguration([]
    {
        auto result = std::make_shared<mo::DefaultConfiguration>(::argc, ::argv);

        namespace po = boost::program_options;

        result->add_options()
                ("tests-use-real-graphics", po::value<bool>()->default_value(false), "Use real graphics in tests.")
                ("tests-use-real-input", po::value<bool>()->default_value(false), "Use real input in tests.");

        return result;
    }())
{
}

std::shared_ptr<mg::Platform> mtf::StubbedServerConfiguration::the_graphics_platform()
{
    if (!graphics_platform)
    {
        graphics_platform = std::make_shared<StubGraphicPlatform>();
    }

    return graphics_platform;
}

std::shared_ptr<mc::RendererFactory> mtf::StubbedServerConfiguration::the_renderer_factory()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-graphics"))
        return DefaultServerConfiguration::the_renderer_factory();
    else
        return renderer_factory(
            [&]()
            {
                return std::make_shared<StubRendererFactory>();
            });
}

std::shared_ptr<mi::InputConfiguration> mtf::StubbedServerConfiguration::the_input_configuration()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_configuration();
    else
        return std::make_shared<mi::NullInputConfiguration>();
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
