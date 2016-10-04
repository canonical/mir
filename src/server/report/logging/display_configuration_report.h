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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#ifndef MIR_DISPLAYCONFIGURATIONREPORT_H
#define MIR_DISPLAYCONFIGURATIONREPORT_H

#include "mir/graphics/display_configuration_observer.h"

#include <memory>

namespace mir
{
namespace logging { class Logger; }

namespace report
{
namespace logging
{
class DisplayConfigurationReport : public mir::graphics::DisplayConfigurationObserver
{
public:
    DisplayConfigurationReport(std::shared_ptr<mir::logging::Logger> const& logger);
    ~DisplayConfigurationReport();

    void configuration_applied(graphics::DisplayConfiguration const& config) override;

    void configuration_failed(graphics::DisplayConfiguration const& attempted, std::exception const& error) override;

    void initial_configuration(graphics::DisplayConfiguration const& configuration) override;

    void catastrophic_configuration_error(
        graphics::DisplayConfiguration const& failed_fallback,
        std::exception const& error) override;

private:
    void log_configuration(graphics::DisplayConfiguration const& configuration) const;
    std::shared_ptr<mir::logging::Logger> const logger;
};
}
}
}

#endif //MIR_DISPLAYCONFIGURATIONREPORT_H
