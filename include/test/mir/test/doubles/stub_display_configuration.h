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
 * Authored by: Kevin DuBois <kevin.dubois@canonical.com>
 */

#ifndef MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_
#define MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_

#include "mir/graphics/display_configuration.h"

#include <vector>

namespace mir
{
namespace test
{
namespace doubles
{

struct StubDisplayConfigurationOutput : public graphics::DisplayConfigurationOutput
{
    StubDisplayConfigurationOutput(
        geometry::Size px_size, geometry::Size mm_size, MirPixelFormat format, double vrefresh, bool connected);

    StubDisplayConfigurationOutput(graphics::DisplayConfigurationOutputId id,
        geometry::Size px_size, geometry::Size mm_size, MirPixelFormat format, double vrefresh, bool connected);
};

class StubDisplayConfig : public graphics::DisplayConfiguration
{
public:
    StubDisplayConfig();

    StubDisplayConfig(StubDisplayConfig const& other);

    StubDisplayConfig(graphics::DisplayConfiguration const& other);

    StubDisplayConfig(unsigned int num_displays);

    StubDisplayConfig(std::vector<std::pair<bool,bool>> const& connected_used);

    StubDisplayConfig(unsigned int num_displays, std::vector<MirPixelFormat> const& pfs);

    StubDisplayConfig(std::vector<geometry::Rectangle> const& rects);

    StubDisplayConfig(std::vector<graphics::DisplayConfigurationOutput> const& outputs);

    StubDisplayConfig(
        std::vector<graphics::DisplayConfigurationCard> const& cards,
        std::vector<graphics::DisplayConfigurationOutput> const& outputs);

    void for_each_card(std::function<void(graphics::DisplayConfigurationCard const&)> f) const override;

    void for_each_output(std::function<void(graphics::DisplayConfigurationOutput const&)> f) const override;

    void for_each_output(std::function<void(graphics::UserDisplayConfigurationOutput&)> f) override;

    std::unique_ptr<graphics::DisplayConfiguration> clone() const override;

    std::vector<graphics::DisplayConfigurationCard> cards;
    std::vector<graphics::DisplayConfigurationOutput> outputs;
};

}
}
}

#endif /* MIR_TEST_DOUBLES_STUB_DISPLAY_CONFIGURATION_H_ */
