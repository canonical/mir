/*
 * Copyright © 2014 Canonical Ltd.
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
 * Authored By: Alan Griffiths <alan@octopull.co.uk>
 */

#include "stubbed_graphics_platform.h"
#include "mir_test_framework/stub_graphics_platform_operation.h"

#include "mir/graphics/buffer_ipc_message.h"

#include "mir_test_framework/stub_platform_helpers.h"

#include "mir/test/doubles/stub_buffer_allocator.h"
#include "mir/test/doubles/stub_display.h"
#include "mir/fd.h"
#include "mir/test/pipe.h"

#ifdef ANDROID
#include "mir/test/doubles/stub_android_native_buffer.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include <system_error>
#include <boost/exception/errinfo_errno.hpp>
#include <boost/throw_exception.hpp>

namespace geom = mir::geometry;
namespace mg = mir::graphics;
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
#if defined(MESA_KMS) || defined(MESA_X11)
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

class StubIpcOps : public mg::PlatformIpcOperations
{
    void pack_buffer(
        mg::BufferIpcMessage& message,
        mg::Buffer const& buffer,
        mg::BufferIpcMsgType msg_type) const override
    {
        if (msg_type == mg::BufferIpcMsgType::full_msg)
        {
#if defined(MESA_KMS) || defined(MESA_X11)
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
        auto package = std::make_shared<mg::PlatformIPCPackage>();
        mtf::pack_stub_ipc_package(*package);
        return package;
    }

    mg::PlatformOperationMessage platform_operation(
         unsigned int const opcode, mg::PlatformOperationMessage const& message) override
    {
        mg::PlatformOperationMessage reply;

        if (opcode == static_cast<unsigned int>(mtf::StubGraphicsPlatformOperation::add))
        {
            if (message.data.size() != 2 * sizeof(int))
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Invalid parameters for 'add' platform operation"));
            }

            auto const int_data = reinterpret_cast<int const*>(message.data.data());

            reply.data.resize(sizeof(int));
            *(reinterpret_cast<int*>(reply.data.data())) = int_data[0] + int_data[1];
        }
        else if (opcode == static_cast<unsigned int>(mtf::StubGraphicsPlatformOperation::echo_fd))
        {
            if (message.fds.size() != 1)
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Invalid parameters for 'echo_fd' platform operation"));
            }

            mir::Fd const request_fd{message.fds[0]};
            char request_char{0};
            if (read(request_fd, &request_char, 1) != 1)
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Failed to read character from request fd in 'echo_fd' operation"));
            }

            mir::test::Pipe pipe;

            if (write(pipe.write_fd(), &request_char, 1) != 1)
            {
                BOOST_THROW_EXCEPTION(
                    std::runtime_error("Failed to write to pipe in 'echo_fd' operation"));
            }

            reply.fds.push_back(dup(pipe.read_fd()));
        }
        else
        {
            BOOST_THROW_EXCEPTION(
                std::runtime_error("Invalid platform operation"));
        }

        return reply;
    }
};
}

mtf::StubGraphicPlatform::StubGraphicPlatform(std::vector<geom::Rectangle> const& display_rects)
    : display_rects{display_rects}
{
}

std::shared_ptr<mg::GraphicBufferAllocator> mtf::StubGraphicPlatform::create_buffer_allocator()
{
    return std::make_shared<StubGraphicBufferAllocator>();
}

std::shared_ptr<mg::PlatformIpcOperations> mtf::StubGraphicPlatform::make_ipc_operations() const
{
    return std::make_shared<StubIpcOps>();
}

namespace
{
std::shared_ptr<mg::Display> display_preset;
}

std::shared_ptr<mg::Display> mtf::StubGraphicPlatform::create_display(
    std::shared_ptr<mg::DisplayConfigurationPolicy> const&,
    std::shared_ptr<mg::GLProgramFactory> const&,
    std::shared_ptr<mg::GLConfig> const&)
{
    if (display_preset)
        return std::move(display_preset);

    return std::make_shared<mtd::StubDisplay>(display_rects);
}

namespace
{
struct GuestPlatformAdapter : mg::Platform
{
    GuestPlatformAdapter(
        std::shared_ptr<mg::NestedContext> const& context,
        std::shared_ptr<mg::Platform> const& adaptee) :
        context(context),
        adaptee(adaptee),
        ipc_ops(adaptee->make_ipc_operations())
    {
    }

    std::shared_ptr<mg::GraphicBufferAllocator> create_buffer_allocator() override
    {
        return adaptee->create_buffer_allocator();
    }

    std::shared_ptr<mg::PlatformIpcOperations> make_ipc_operations() const override
    {
        return ipc_ops;
    }

    std::shared_ptr<mg::Display> create_display(
        std::shared_ptr<mg::DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<mg::GLProgramFactory> const& gl_program_factory,
        std::shared_ptr<mg::GLConfig> const& gl_config) override
    {
        return adaptee->create_display(initial_conf_policy, gl_program_factory, gl_config);
    }

    EGLNativeDisplayType egl_native_display() const override
    {
        return adaptee->egl_native_display();
    }

    std::shared_ptr<mg::NestedContext> const context;
    std::shared_ptr<mg::Platform> const adaptee;
    std::shared_ptr<mg::PlatformIpcOperations> const ipc_ops;
};

std::weak_ptr<mg::Platform> the_graphics_platform{};
std::unique_ptr<std::vector<geom::Rectangle>> chosen_display_rects;
}

extern "C" std::shared_ptr<mg::Platform> create_stub_platform(std::vector<geom::Rectangle> const& display_rects)
{
    return std::make_shared<mtf::StubGraphicPlatform>(display_rects);
}

std::shared_ptr<mg::Platform> create_host_platform(
    std::shared_ptr<mo::Option> const& /*options*/,
    std::shared_ptr<mir::EmergencyCleanupRegistry> const& /*emergency_cleanup_registry*/,
    std::shared_ptr<mg::DisplayReport> const& /*report*/)
{
    std::shared_ptr<mg::Platform> result{};

    if (auto const display_rects = std::move(chosen_display_rects))
    {
        result = create_stub_platform(*display_rects);
    }
    else
    {
        static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
        result = create_stub_platform(default_display_rects);
    }
    the_graphics_platform = result;
    return result;
}

std::shared_ptr<mg::Platform> create_guest_platform(
    std::shared_ptr<mg::DisplayReport> const&,
    std::shared_ptr<mg::NestedContext> const& context)
{
    auto graphics_platform = the_graphics_platform.lock();
    if (!graphics_platform)
    {
        static std::vector<geom::Rectangle> const default_display_rects{geom::Rectangle{{0,0},{1600,1600}}};
        the_graphics_platform = graphics_platform = create_stub_platform(default_display_rects);
    }
    return std::make_shared<GuestPlatformAdapter>(context, graphics_platform);
}

void add_graphics_platform_options(
    boost::program_options::options_description& /*config*/)
{
}

extern "C" void set_next_display_rects(
    std::unique_ptr<std::vector<geom::Rectangle>>&& display_rects)
{
    chosen_display_rects = std::move(display_rects);
}

extern "C" void set_next_preset_display(std::shared_ptr<mir::graphics::Display> const& display)
{
    display_preset = display;
}
