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
#include "mir_test_framework/command_line_server_configuration.h"

#include "mir/options/default_configuration.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/buffer_ipc_message.h"
#include "mir/graphics/buffer_writer.h"
#include "mir/graphics/cursor.h"
#include "mir/input/input_channel.h"
#include "mir/input/input_manager.h"

#include "mir_test_doubles/stub_display.h"
#include "mir_test_doubles/null_platform.h"
#include "mir_test_doubles/stub_buffer.h"
#include "mir_test_doubles/stub_buffer_allocator.h"
#include "mir_test_doubles/stub_display_buffer.h"
#include "mir_test_doubles/stub_renderer.h"
#include "mir_test_doubles/stub_input_sender.h"

#ifdef ANDROID
#include "mir_test_doubles/mock_android_native_buffer.h"
#endif

#include "mir/compositor/renderer.h"
#include "mir/compositor/renderer_factory.h"
#include "src/server/input/null_input_configuration.h"
#include "src/server/input/null_input_dispatcher.h"
#include "src/server/input/null_input_targeter.h"

#include <system_error>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

namespace geom = mir::geometry;
namespace mc = mir::compositor;
namespace msh = mir::shell;
namespace mg = mir::graphics;
namespace mi = mir::input;
namespace mo = mir::options;
namespace mtd = mir::test::doubles;
namespace mtf = mir_test_framework;

namespace
{
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
                    std::system_error(errno, std::system_category(), "Failed to open dummy fd")));
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
        if (properties.size.width == geom::Width{0} ||
            properties.size.height == geom::Height{0})
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Request for allocation of buffer with invalid size"));
        }

        return std::make_shared<StubFDBuffer>(properties);
    }
};

class StubCursor : public mg::Cursor
{
    void show(mg::CursorImage const&) override {}
    void hide() override {}
    void move_to(geom::Point) override {}
};

class StubIpcOps : public mg::PlatformIpcOperations
{
    void pack_buffer(
        mg::BufferIpcMessage& message,
        mg::Buffer const& buffer,
        mg::BufferIpcMsgType msg_type) const override
    {
        if (msg_type == mg::BufferIpcMsgType::full_msg)
        {
#ifndef ANDROID
            auto native_handle = buffer.native_buffer_handle();
            for(auto i=0; i<native_handle->data_items; i++)
            {
                message.pack_data(native_handle->data[i]);
            }
            for(auto i=0; i<native_handle->fd_items; i++)
            {
                using namespace mir;
                message.pack_fd(Fd(IntOwnedFd{native_handle->fd[i]}));
            }

            message.pack_flags(native_handle->flags);
#endif
            message.pack_stride(buffer.stride());
            message.pack_size(buffer.size());
        }
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

class StubGraphicPlatform : public mtd::NullPlatform
{
public:
    StubGraphicPlatform(std::vector<geom::Rectangle> const& display_rects)
        : display_rects{display_rects}
    {
    }

    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator(
        const std::shared_ptr<mg::BufferInitializer>& /*buffer_initializer*/) override
    {
        return std::make_shared<StubGraphicBufferAllocator>();
    }

    std::shared_ptr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        return std::make_shared<StubIpcOps>();
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
        std::shared_ptr<mg::GLProgramFactory> const&,
        std::shared_ptr<mg::GLConfig> const&) override
    {
        return std::make_shared<mtd::StubDisplay>(display_rects);
    }
    
    std::shared_ptr<mg::BufferWriter> make_buffer_writer() override
    {
        struct NullWriter : mg::BufferWriter 
        {
            void write(mg::Buffer& /* buffer */, 
                unsigned char const* /* data */, size_t /* size */) override
            {
            }
        };
        return std::make_shared<NullWriter>();
    }
    
    std::vector<geom::Rectangle> const display_rects;
};

class StubRendererFactory : public mc::RendererFactory
{
public:
    std::unique_ptr<mc::Renderer> create_renderer_for(geom::Rectangle const&, mc::DestinationAlpha)
    {
        return std::unique_ptr<mc::Renderer>(new mtd::StubRenderer());
    }
};

}

mtf::StubbedServerConfiguration::StubbedServerConfiguration()
    : StubbedServerConfiguration({geom::Rectangle{{0,0},{1600,1600}}})
{
}

mtf::StubbedServerConfiguration::StubbedServerConfiguration(
    std::vector<geom::Rectangle> const& display_rects)
    : DefaultServerConfiguration([]
      {
          auto result = mtf::configuration_from_commandline();

          namespace po = boost::program_options;

          result->add_options()
                  ("tests-use-real-graphics", po::value<bool>()->default_value(false), "Use real graphics in tests.")
                  ("tests-use-real-input", po::value<bool>()->default_value(false), "Use real input in tests.");

          return result;
      }()),
      display_rects{display_rects}
{
}

std::shared_ptr<mg::Platform> mtf::StubbedServerConfiguration::the_graphics_platform()
{
    if (!graphics_platform)
    {
        graphics_platform = std::make_shared<StubGraphicPlatform>(display_rects);
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

std::shared_ptr<msh::InputTargeter> mtf::StubbedServerConfiguration::the_input_targeter()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_targeter();
    else
        return std::make_shared<mi::NullInputTargeter>();
}

std::shared_ptr<mi::InputDispatcher> mtf::StubbedServerConfiguration::the_input_dispatcher()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_dispatcher();
    else
        return std::make_shared<mi::NullInputDispatcher>();
}

std::shared_ptr<mi::InputSender> mtf::StubbedServerConfiguration::the_input_sender()
{
    auto options = the_options();

    if (options->get<bool>("tests-use-real-input"))
        return DefaultServerConfiguration::the_input_sender();
    else
        return std::make_shared<mtd::StubInputSender>();
}

std::shared_ptr<mg::Cursor> mtf::StubbedServerConfiguration::the_cursor()
{
    return std::make_shared<StubCursor>();
}
