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
#include "ipc_operations.h"
#include "mesa_extensions.h"
#include "mir/options/option.h"

namespace mo = mir::options;
namespace mg = mir::graphics;
namespace mgm = mg::mesa;
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

auto parse_size(std::string const& str) -> geom::Size
{
    size_t x = str.find('x');
    if (x == std::string::npos || x <= 0 || x >= str.size() - 1)
        BOOST_THROW_EXCEPTION(std::runtime_error("Output size \"" + str + "\" does not have two dimensions"));
    return geom::Size{
        parse_size_dimension(str.substr(0, x)),
        parse_size_dimension(str.substr(x + 1, std::string::npos))};

}
}

std::vector<geom::Size> mgx::Platform::parse_output_sizes(std::string output_sizes)
{
    std::vector<geom::Size> sizes;
    for (int start = 0, end; start - 1 < (int)output_sizes.size(); start = end + 1)
    {
        end = output_sizes.find(',', start);
        if (end == (int)std::string::npos)
            end = output_sizes.size();
        sizes.push_back(parse_size(output_sizes.substr(start, end - start)));
    }
    return sizes;
}

mgx::Platform::Platform(std::shared_ptr<::Display> const& conn,
                        std::vector<geom::Size> const output_sizes,
                        std::shared_ptr<mg::DisplayReport> const& report)
    : x11_connection{conn},
      udev{std::make_shared<mir::udev::Context>()},
      drm{mgm::helpers::DRMHelper::open_any_render_node(udev)},
      report{report},
      gbm{drm->fd},
      output_sizes{output_sizes}
{
    if (!x11_connection)
        BOOST_THROW_EXCEPTION(std::runtime_error("Need valid x11 display"));

    auth_factory = std::make_unique<mgm::DRMNativePlatformAuthFactory>(*drm);
}

mir::UniqueModulePtr<mg::GraphicBufferAllocator> mgx::Platform::create_buffer_allocator(
    mg::Display const& output)
{
    return make_module_ptr<mgm::BufferAllocator>(output, gbm.device, mgm::BypassOption::prohibited, mgm::BufferImportMethod::dma_buf);
}

mir::UniqueModulePtr<mg::Display> mgx::Platform::create_display(
    std::shared_ptr<DisplayConfigurationPolicy> const& /*initial_conf_policy*/,
    std::shared_ptr<GLConfig> const& gl_config)
{
    return make_module_ptr<mgx::Display>(x11_connection.get(), output_sizes, gl_config, report);
}

mg::NativeDisplayPlatform* mgx::Platform::native_display_platform()
{
    return auth_factory.get();
}

mir::UniqueModulePtr<mg::PlatformIpcOperations> mgx::Platform::make_ipc_operations() const
{
    return make_module_ptr<mg::mesa::IpcOperations>(drm);
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
    return mgm::mesa_extensions();
}
