/*
 * Copyright © 2016 Canonical Ltd.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 or 3,
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
#include <memory>

namespace mir
{
namespace scene
{
class Session;
}
namespace graphics
{
class DisplayConfiguration;

class DisplayConfigurationObserver
{
public:
    /**
     * Notification of the initial display configuration.
     *
     * This is called exactly once, at server startup.
     *
     * \param [in] config   The initial configuration
     */
    virtual void initial_configuration(std::shared_ptr<DisplayConfiguration const> const& config) = 0;

    /**
     * Notification after every successful display configuration.
     *
     * This is called once per successful display configuration, after it has been applied
     * to the hardware.
     *
     * \param [in] config   The configuration that has just been applied.
     */
    virtual void configuration_applied(std::shared_ptr<DisplayConfiguration const> const& config) = 0;

    /**
     * Notification after updating base display configuration.
     *
     * \param [in] base_config   The configuration that has just been updated.
     */
    virtual void base_configuration_updated(std::shared_ptr<DisplayConfiguration const> const& base_config) = 0;

    /**
     * Notification after updating the session display configuration.
     *
     * \param [in] session  The session that is updating its display configuration.
     * \param [in] config   The configuration that is being applied to the session.
     */
    virtual void session_configuration_applied(std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<DisplayConfiguration> const& config) = 0;
    /**
     * Notification after removing the session display configuration.
     *
     * \param [in] session  The session that is removing its display configuration.
     */
    virtual void session_configuration_removed(std::shared_ptr<scene::Session> const& session) = 0;

    /**
     * Notification after every failed display configuration attempt.
     *
     * This is called if the graphics platform throws an exception when trying
     * to configure the hardware.
     *
     * In this case the previous display configuration has been re-applied.
     *
     * \param [in] attempted    The display configuration that failed to apply.
     * \param [in] error        The exception thrown by the graphics platform.
     */
    virtual void configuration_failed(
        std::shared_ptr<DisplayConfiguration const> const& attempted,
        std::exception const& error) = 0;

    /**
     * Notification after a failed display configuration with failed recovery.
     *
     * This is called if the graphics platform throws an exception when trying
     * to configure the hardware \b and has thrown an exception when trying
     * to re-apply the previous configuration.
     *
     * When this call is made it is unknown what state the display hardware is in,
     * but it's unlikely to be displaying any image to the user.
     *
     * As there is no sensible behaviour for Mir to have in this case, the shell
     * should respond to this either by trying to guess a safe configuration,
     * by switching to some other display mechanism, or by terminating.
     *
     * \param [in] failed_fallback  The fallback display configuration that failed to apply.
     * \param error                 The exception thrown by the graphics platform.
     */
    virtual void catastrophic_configuration_error(
        std::shared_ptr<DisplayConfiguration const> const& failed_fallback,
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
