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

#include "display_configuration_report.h"
#include "mir/graphics/display_configuration.h"
#include "mir/output_type_names.h"
#include "mir/logging/logger.h"

#include <boost/exception/diagnostic_information.hpp>
#include <cmath>

namespace mg = mir::graphics;
namespace ml = mir::logging;
namespace mrl= mir::report::logging;

namespace 
{
auto const component = MIR_LOG_COMPONENT_FALLBACK;
auto const severity  = ml::Severity::informational;
}

mrl::DisplayConfigurationReport::DisplayConfigurationReport(std::shared_ptr<ml::Logger> const& logger) :
    logger{logger}
{
}

mrl::DisplayConfigurationReport::~DisplayConfigurationReport()
{
}

void mrl::DisplayConfigurationReport::initial_configuration(mg::DisplayConfiguration const& configuration)
{
    logger->log(component, severity, "Initial display configuration:");
    log_configuration(configuration);
}

void mrl::DisplayConfigurationReport::configuration_applied(
    mg::DisplayConfiguration const& config)
{
    logger->log(component, severity, "New display configuration:");
    log_configuration(config);
}

void mrl::DisplayConfigurationReport::configuration_failed(
    mg::DisplayConfiguration const& attempted,
    std::exception const& error)
{
    logger->log(component, ml::Severity::error, "Failed to apply display configuration:");
    log_configuration(attempted);
    logger->log(component, ml::Severity::error, "Error details:");
    logger->log(component, ml::Severity::error, "%s", boost::diagnostic_information(error).c_str());
}

void mrl::DisplayConfigurationReport::log_configuration(mg::DisplayConfiguration const& configuration) const
{
    configuration.for_each_output([this](mg::DisplayConfigurationOutput const& out)
    {
        auto type = mir::output_type_name(static_cast<unsigned>(out.type));
        int out_id = out.id.as_value();
        char const indent[] = "      ";

        logger->log(component, severity,
                    "  Output %d: %s %s",
                    out_id, type,
                    !out.connected ? "disconnected" :
                                     out.used ? "connected, used" :
                                                "connected, unused"
                    );
        if (out.connected)
        {
            int width_mm = out.physical_size_mm.width.as_int();
            int height_mm = out.physical_size_mm.height.as_int();
            float inches =
                sqrtf(width_mm * width_mm + height_mm * height_mm) / 25.4;

            logger->log(component, severity, 
                        "%sPhysical size %.1f\" %dx%dmm",
                        indent, inches, width_mm, height_mm);
            
            if (out.used)
            {
                if (out.current_mode_index < out.modes.size())
                {
                    auto const& mode = out.modes[out.current_mode_index];
                    logger->log(component, severity,
                                "%sCurrent mode %dx%d %.2fHz",
                                indent,
                                mode.size.width.as_int(),
                                mode.size.height.as_int(),
                                mode.vrefresh_hz);
                }
                
                if (out.preferred_mode_index < out.modes.size())
                {
                    auto const& mode = out.modes[out.preferred_mode_index];
                    logger->log(component, severity,
                                "%sPreferred mode %dx%d %.2fHz",
                                indent,
                                mode.size.width.as_int(),
                                mode.size.height.as_int(),
                                mode.vrefresh_hz);
                }
                
                logger->log(component, severity,
                            "%sLogical position %+d%+d",
                            indent,
                            out.top_left.x.as_int(),
                            out.top_left.y.as_int());
            }
        }
    });
}
