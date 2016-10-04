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
 * Authored by: Christopher James Halse Rogers <christopher.halse.rogers@canonical.com>
 */

#ifndef MIR_GRAPHICS_DISPLAY_CONFIGURATION_OBSERVER_
#define MIR_GRAPHICS_DISPLAY_CONFIGURATION_OBSERVER_

#include <exception>

namespace mir
{
namespace graphics
{
class DisplayConfiguration;

class DisplayConfigurationObserver
{
public:
    virtual void initial_configuration(DisplayConfiguration const& config) = 0;
    virtual void configuration_applied(DisplayConfiguration const& config) = 0;
    virtual void configuration_failed(
        DisplayConfiguration const& attempted,
        std::exception const& error) = 0;

protected:
    DisplayConfigurationObserver() = default;
    virtual ~DisplayConfigurationObserver() = default;
    DisplayConfigurationObserver(DisplayConfigurationObserver const&) = delete;
    DisplayConfigurationObserver& operator=(DisplayConfigurationObserver const&) = delete;
};
}
}

#endif //MIR_GRAPHICS_DISPLAY_CONFIGURATION_OBSERVER_
