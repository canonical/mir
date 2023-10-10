/*
 * Copyright © Canonical Ltd.
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
 */

#ifndef MIR_GRAPHICS_VIRTUAL_PLATFORM_H_
#define MIR_GRAPHICS_VIRTUAL_PLATFORM_H_

#include "mir/graphics/platform.h"
#include "mir/options/option.h"
#include "mir/graphics/display.h"
#include "mir/fd.h"

namespace mir
{
namespace graphics
{
namespace virt
{

struct VirtualOutputConfig
{
    VirtualOutputConfig(std::vector<geometry::Size> sizes)
        : sizes{sizes}
    {
    }

    bool operator==(VirtualOutputConfig const& output) const
    {
        return sizes == output.sizes;
    }
    std::vector<geometry::Size> sizes;
};

class Platform : public graphics::DisplayPlatform
{
public:
    static auto parse_output_sizes(std::vector<std::string> virtual_outputs) -> std::vector<VirtualOutputConfig>;
    Platform(
        std::shared_ptr<DisplayReport> const& report,
        std::vector<VirtualOutputConfig> outputs);
    ~Platform();

    UniqueModulePtr<Display> create_display(
        std::shared_ptr<DisplayConfigurationPolicy> const& initial_conf_policy,
        std::shared_ptr<GLConfig> const& gl_config) override;

    static bool has_instance();

protected:
    auto interface_for() -> std::shared_ptr<DisplayInterfaceProvider> override;
private:
    class VirtualDisplayInterfaceProvider;
    std::shared_ptr<DisplayReport> const report;
    std::shared_ptr<VirtualDisplayInterfaceProvider> const provider;
    std::vector<VirtualOutputConfig> const outputs;
};
}
}
}

#endif // MIR_GRAPHICS_VIRTUAL_PLATFORM_H_
