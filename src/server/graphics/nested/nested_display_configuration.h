/*
 * Copyright © 2013 Canonical Ltd.
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
 * Authored by: Eleni Maria Stea <elenimaria.stea@canonical.com>
 */

#ifndef MIR_GRAPHICS_NESTED_NESTED_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_NESTED_NESTED_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include "mir_api_wrappers.h"

namespace mir
{
namespace graphics
{
namespace nested
{
class NestedDisplayConfiguration : public DisplayConfiguration
{
public:
    explicit NestedDisplayConfiguration(MirDisplayConfiguration* display_config);
    virtual ~NestedDisplayConfiguration() noexcept;

    void for_each_card(std::function<void(DisplayConfigurationCard const&)>) const override;
    void for_each_output(std::function<void(DisplayConfigurationOutput const&)>) const override;

    void configure_output(DisplayConfigurationOutputId id, bool used, geometry::Point top_left, size_t mode_index,
                          MirPixelFormat format, MirPowerMode power_mode) override;

    operator MirDisplayConfiguration*() const { return display_config; }
private:
    mir_api_wrappers::MirDisplayConfigHandle display_config;
};
}
}
}

#endif // MIR_GRAPHICS_NESTED_NESTED_DISPLAY_CONFIGURATION_H_
