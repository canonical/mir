/*
 * Copyright © 2016 Canonical Ltd.
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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_PLATFORM_H_
#define MIR_GRAPHICS_NESTED_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "passthrough_option.h"
#include <memory>

namespace mir
{
namespace options
{
class Option;
}
namespace graphics
{
class DisplayReport;
namespace nested
{
class HostConnection;
class Platform : public graphics::Platform
{
public:
    Platform(
        std::shared_ptr<mir::SharedLibrary> const& library, 
        std::shared_ptr<HostConnection> const& connection, 
        std::shared_ptr<DisplayReport> const& display_report,
        options::Option const& options);

    UniqueModulePtr<GraphicBufferAllocator> create_buffer_allocator() override;
    UniqueModulePtr<graphics::Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) override;
    UniqueModulePtr<PlatformIpcOperations> make_ipc_operations() const override;
private:
    std::shared_ptr<mir::SharedLibrary> const library; 
    std::shared_ptr<HostConnection> const connection; 
    std::shared_ptr<DisplayReport> const display_report;

    //the concept of guest platform is strange, it only exists to deny creating a
    //host display in a nested context. It should go away soon.
    std::shared_ptr<graphics::Platform> const guest_platform;
    PassthroughOption const passthrough_option;
};
}
}
}

#endif /* MIR_GRAPHICS_NESTED_PLATFORM_H_ */
