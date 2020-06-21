/*
 * Copyright © 2015 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License version 2 or 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Cemil Azizoglu <cemil.azizoglu@canonical.com>
 */

#include "platform.h"
#include "display.h"
#include "buffer_allocator.h"
#include "mir/options/option.h"
#include "mir/graphics/platform_ipc_operations.h"
#include "mir/graphics/platform_operation_message.h"


namespace mo = mir::options;
namespace mg = mir::graphics;
namespace mgx = mg::X;
namespace geom = mir::geometry;

namespace
{
auto parse_size_dimension(std::string const& str) -> int
{
    try
    {
        size_t num_end = 0;
        int const value = std::stoi(str, &num_end);
        if (num_end != str.size())
            BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is not a valid number"));
        if (value <= 0)
            BOOST_THROW_EXCEPTION(std::runtime_error("Output dimensions must be greater than zero"));
        return value;
    }
    catch (std::invalid_argument const &e)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Output dimension \"" + str + "\" is not a valid number"));
    }
}

auto parse_scale(std::string const& str) -> float
{
    try
    {
        size_t num_end = 0;
        float const value = std::stof(str, &num_end);
        if (num_end != str.size())
            BOOST_THROW_EXCEPTION(std::runtime_error("Scale \"" + str + "\" is not a valid float"));
        if (value <= 0.000001)
            BOOST_THROW_EXCEPTION(std::runtime_error("Scale must be greater than zero"));
        return value;
    }
    catch (std::invalid_argument const &e)
    {
        BOOST_THROW_EXCEPTION(std::runtime_error("Scale \"" + str + "\" is not a valid float"));
    }
}

auto parse_size(std::string const& str) -> mgx::X11OutputConfig
{
    auto const x = str.find('x'); // "x" between width and height
    if (x == std::string::npos || x <= 0 || x >= str.size() - 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output size \"" + str + "\" does not have two dimensions"));
    auto const scale_start = str.find('^'); // start of output scale
    float scale = 1.0f;
    if (scale_start != std::string::npos)
    {
        if (scale_start >= str.size() - 1)
            BOOST_THROW_EXCEPTION(std::runtime_error("In \"" + str + "\", '^' is not followed by a scale"));
        scale = parse_scale(str.substr(scale_start + 1, std::string::npos));
    }
    return mgx::X11OutputConfig{
        geom::Size{
            parse_size_dimension(str.substr(0, x)),
            parse_size_dimension(str.substr(x + 1, scale_start - x - 1))},
        scale};
}
}

auto mgx::Platform::parse_output_sizes(std::string output_sizes) -> std::vector<mgx::X11OutputConfig>
{
    std::vector<mgx::X11OutputConfig> sizes;
    for (int start = 0, end; start - 1 < (int)output_sizes.size(); start = end + 1)
    {
        end = output_sizes.find(':', start);
        if (end == (int)std::string::npos)
            end = output_sizes.size();
        sizes.push_back(parse_size(output_sizes.substr(start, end - start)));
    }
    return sizes;
}

mgx::Platform::Platform(std::shared_ptr<::Display> const& conn,
                        std::vector<X11OutputConfig> output_sizes,
                        std::shared_ptr<mg::DisplayReport> const& report)
    : x11_connection{conn},
      report{report},
      output_sizes{move(output_sizes)}
{
    if (!x11_connection)
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 display"));
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator(
    mg::Display const& output)
{
    return make_module_ptr<mgx::BufferAllocator>(output);
}

mir::UniqueModulePtr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return make_module_ptr<mgx::Display>(x11_connection.get(), output_sizes, initial_conf_policy, gl_config, report);
}

mg::NativeDisplayPlatform* mgx::Platform::native_display_platform()
{
    return nullptr;
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    class NoIPCOperations : public mg::PlatformIpcOperations
    {

    public:
        void pack_buffer(BufferIpcMessage&, Buffer const&, BufferIpcMsgType) const override
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient unsupported by X11 platform"}));
        }

        void unpack_buffer(BufferIpcMessage& /*message*/, Buffer const& /*buffer*/) const override
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient unsupported by X11 platform"}));
        }

        std::shared_ptr<mg::PlatformIPCPackage> connection_ipc_package() override
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient unsupported by X11 platform"}));
        }

        PlatformOperationMessage platform_operation(unsigned int const /*opcode*/,
                                                    PlatformOperationMessage const& /*message*/) override
        {
            BOOST_THROW_EXCEPTION((std::runtime_error{"mirclient unsupported by X11 platform"}));
        }
    };

    return mir::make_module_ptr<NoIPCOperations>();
}

mg::NativeRenderingPlatform* mgx::Platform::native_rendering_platform()
{
    return this;
}

EGLNativeDisplayType mgx::Platform::egl_native_display() const
{
    return eglGetDisplay(x11_connection.get());
}

std::vector<mir::ExtensionDescription> mgx::Platform::extensions() const
{
    return {};
}
