/*
 * Copyright © 2013 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3,
 * as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * Authored by: Alexandros Frantzis <alexandros.frantzis@canonical.com>
 */

#ifndef MIR_GRAPHICS_DISPLAY_CONFIGURATION_H_
#define MIR_GRAPHICS_DISPLAY_CONFIGURATION_H_

#include "mir/int_wrapper.h"
#include "mir/geometry/size.h"

#include <functional>
#include <vector>

namespace mir
{
namespace graphics
{

typedef IntWrapper<IntWrapperTypeTag::GraphicsConfCardId> DisplayConfigurationCardId;
typedef IntWrapper<IntWrapperTypeTag::GraphicsConfOutputId> DisplayConfigurationOutputId;

struct DisplayConfigurationCard
{
    DisplayConfigurationCardId id;
};

struct DisplayConfigurationMode
{
    geometry::Size size;
    double vrefresh_hz;
};

struct DisplayConfigurationOutput
{
    DisplayConfigurationOutputId id;
    DisplayConfigurationCardId card_id;
    std::vector<DisplayConfigurationMode> modes;
    geometry::Size physical_size_mm;
    bool connected;
    size_t current_mode_index;
};

std::ostream& operator<<(std::ostream& out, DisplayConfigurationMode const& val);
bool operator==(DisplayConfigurationMode const& val1, DisplayConfigurationMode const& val2);
bool operator!=(DisplayConfigurationMode const& val1, DisplayConfigurationMode const& val2);

std::ostream& operator<<(std::ostream& out, DisplayConfigurationOutput const& val);
bool operator==(DisplayConfigurationOutput const& val1, DisplayConfigurationOutput const& val2);
bool operator!=(DisplayConfigurationOutput const& val1, DisplayConfigurationOutput const& val2);

class DisplayConfiguration
{
public:
    virtual ~DisplayConfiguration() {}

    virtual void for_each_card(std::function<void(DisplayConfigurationCard const&)> f) = 0;
    virtual void for_each_output(std::function<void(DisplayConfigurationOutput const&)> f) = 0;

protected:
    DisplayConfiguration() = default;
    DisplayConfiguration(DisplayConfiguration const& c) = delete;
    DisplayConfiguration& operator=(DisplayConfiguration const& c) = delete;
};

}
}

#endif /* MIR_GRAPHICS_DISPLAY_CONFIGURATION_H_ */
