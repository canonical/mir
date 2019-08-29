/*
 * Copyright © 2015 Canonical Ltd.
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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_DISPLAYCONFIGURATIONREPORT_H
#define MIR_DISPLAYCONFIGURATIONREPORT_H

#include "mir/graphics/display_configuration_observer.h"

#include <memory>

namespace mir
{
namespace logging { class Logger; enum class Severity; }

namespace frontend
{
class Session;
}
namespace report
{
namespace logging
{
class DisplayConfigurationReport : public mir::graphics::DisplayConfigurationObserver
{
public:
    DisplayConfigurationReport(std::shared_ptr<mir::logging::Logger> const& logger);
    ~DisplayConfigurationReport();

    void configuration_applied(
        std::shared_ptr<graphics::DisplayConfiguration const> const& config) override;

    void base_configuration_updated(std::shared_ptr<graphics::DisplayConfiguration const> const& base_config) override;

    void session_configuration_applied(std::shared_ptr<scene::Session> const& session,
                                       std::shared_ptr<graphics::DisplayConfiguration> const& config) override;

    void session_configuration_removed(std::shared_ptr<scene::Session> const& session) override;

    void configuration_failed(
        std::shared_ptr<graphics::DisplayConfiguration const> const& attempted,
        std::exception const& error) override;

    void initial_configuration(
        std::shared_ptr<graphics::DisplayConfiguration const> const& configuration) override;

    void catastrophic_configuration_error(
        std::shared_ptr<graphics::DisplayConfiguration const> const& failed_fallback,
        std::exception const& error) override;

    void session_should_send_display_configuration(
        std::shared_ptr<scene::Session> const& session,
        std::shared_ptr<graphics::DisplayConfiguration const> const& config) override;

private:
    void log_configuration(
        mir::logging::Severity severity,
        graphics::DisplayConfiguration const& configuration) const;
    std::shared_ptr<mir::logging::Logger> const logger;
};
}
}
}

#endif //MIR_DISPLAYCONFIGURATIONREPORT_H
