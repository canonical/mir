/*
 * Copyright © 2015 Canonical Ltd.
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

#ifndef MIR_SHELL_DISPLAY_CONFIGURATION_CONTROLLER_H_
#define MIR_SHELL_DISPLAY_CONFIGURATION_CONTROLLER_H_

#include <future>
#include <memory>

namespace mir
{
namespace graphics
{
class DisplayConfiguration;
}

namespace shell
{
class DisplayConfigurationController
{
public:
    DisplayConfigurationController() = default;
    virtual ~DisplayConfigurationController() = default;

    DisplayConfigurationController(DisplayConfigurationController const&) = delete;
    DisplayConfigurationController& operator=(DisplayConfigurationController const&) = delete;

    /**
     * Set the base display configuration.
     *
     * This is the display configuration that is used by default, but will be
     * overridden by a client's requested configuration if that client is focused.
     *
     * \param [in]  conf    The new display configuration to set
     * \return              A std::future<> that becomes ready once the configuration
     *                      has been applied.
     */
    virtual std::future<void> set_default_display_configuration(
        std::shared_ptr<graphics::DisplayConfiguration> const& conf) = 0;
};
}
}

#endif //MIR_SHELL_DISPLAY_CONFIGURATION_CONTROLLER_H_
