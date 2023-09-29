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

#ifndef MIR_GRAPHICS_VIRTUAL_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_VIRTUAL_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include <memory>

namespace mir
{
namespace graphics
{
namespace virt
{
class VirtualOutputConfig;
class DisplayConfiguration : public mir::graphics::DisplayConfiguration
{
public:
    static DisplayConfigurationOutput build_output(VirtualOutputConfig const& config);
    DisplayConfiguration(std::vector<DisplayConfigurationOutput> const& outputs);
    DisplayConfiguration(DisplayConfiguration const&);
    virtual ~DisplayConfiguration() = default;

    void for_each_output(std::function<void(const DisplayConfigurationOutput &)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput &)> f) override;
    std::unique_ptr<mir::graphics::DisplayConfiguration> clone() const override;
    bool valid() const override;

private:
    static int last_output_id;
    std::vector<DisplayConfigurationOutput> configuration;
};
}
}
}

#endif //MIR_GRAPHICS_VIRTUAL_DISPLAY_CONFIGURATION_H_
