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
 *
 */

#ifndef MIR_GRAPHICS_X_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_X_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"
#include "mir/geometry/size.h"

namespace mir
{
namespace graphics
{
namespace X
{

struct DisplayConfigurationOutput : graphics::DisplayConfigurationOutput
{
    DisplayConfigurationOutput(
        MirPixelFormat pf,
        mir::geometry::Size const pixels,
        mir::geometry::Size const size_mm,
        float const scale,
        MirOrientation orientation);

    static int next_output_id;
};

class DisplayConfiguration : public graphics::DisplayConfiguration
{
public:
    DisplayConfiguration(std::vector<graphics::DisplayConfigurationOutput> const& outputs);
    DisplayConfiguration(DisplayConfiguration const&);

    virtual ~DisplayConfiguration() = default;

    void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) const override;
    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const override;
    void for_each_output(std::function<void(UserDisplayConfigurationOutput&)> f) override;
    std::unique_ptr<graphics::DisplayConfiguration> clone() const override;

private:
    std::vector<graphics::DisplayConfigurationOutput> configuration;
    DisplayConfigurationCard card;
};


}
}
}
#endif /* MIR_GRAPHICS_X_DISPLAY_CONFIGURATION_H_ */
