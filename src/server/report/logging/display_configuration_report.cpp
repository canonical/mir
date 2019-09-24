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

#include "display_configuration_report.h"
#include "mir/graphics/display_configuration.h"
#include "mir/output_type_names.h"
#include "mir/logging/logger.h"
#include "mir/graphics/edid.h"
#include "mir/scene/session.h"

#include <boost/exception/diagnostic_information.hpp>
#include <cmath>

namespace mf = mir::frontend;
namespace ms = mir::scene;
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

void mrl::DisplayConfigurationReport::initial_configuration(
    std::shared_ptr<mg::DisplayConfiguration const> const& configuration)
{
    logger->log(component, severity, "Initial display configuration:");
    log_configuration(severity, *configuration);
}

void mrl::DisplayConfigurationReport::configuration_applied(
    std::shared_ptr<mg::DisplayConfiguration const> const& config)
{
    logger->log(component, severity, "New display configuration:");
    log_configuration(severity, *config);
}

void mrl::DisplayConfigurationReport::base_configuration_updated(
    std::shared_ptr<mg::DisplayConfiguration const> const& base_config)
{
    logger->log(component, severity, "New base display configuration:");
    log_configuration(severity, *base_config);
}

void mrl::DisplayConfigurationReport::session_configuration_applied(std::shared_ptr<ms::Session> const& session,
                                   std::shared_ptr<mg::DisplayConfiguration> const& config)
{
    logger->log(component, severity, "Session %s applied display configuration", session->name().c_str());
    log_configuration(severity, *config);
}

void mrl::DisplayConfigurationReport::session_configuration_removed(std::shared_ptr<ms::Session> const& session)
{
    logger->log(component, severity, "Session %s removed display configuration", session->name().c_str());
}

void mrl::DisplayConfigurationReport::configuration_failed(
    std::shared_ptr<mg::DisplayConfiguration const> const& attempted,
    std::exception const& error)
{
    logger->log(component, ml::Severity::error, "Failed to apply display configuration:");
    log_configuration(ml::Severity::error, *attempted);
    logger->log(component, ml::Severity::error, "Error details:");
    logger->log(component, ml::Severity::error, "%s", boost::diagnostic_information(error).c_str());
}

void mrl::DisplayConfigurationReport::log_configuration(
    ml::Severity severity,
    mg::DisplayConfiguration const& configuration) const
{
    configuration.for_each_output([this, severity](mg::DisplayConfigurationOutput const& out)
    {
        auto type = mir::output_type_name(static_cast<unsigned>(out.type));
        int out_id = out.id.as_value();
        char const indent[] = ". |_ ";

        logger->log(component, severity,
                    "* Output %d: %s %s",
                    out_id, type,
                    !out.connected ? "disconnected" :
                                     out.used ? "connected, used" :
                                                "connected, unused"
                    );
        if (out.connected)
        {
            using mir::graphics::Edid;
            if (out.edid.size() >= Edid::minimum_size)
            {
                auto edid = reinterpret_cast<Edid const*>(out.edid.data());
                Edid::MonitorName name;
                if (edid->get_monitor_name(name))
                    logger->log(component, severity,
                                "%sEDID monitor name: %s", indent, name);
                Edid::Manufacturer man;
                edid->get_manufacturer(man);
                logger->log(component, severity,
                            "%sEDID manufacturer: %s", indent, man);
                logger->log(component, severity,
                            "%sEDID product code: %hu", indent, edid->product_code());
            }

            int width_mm = out.physical_size_mm.width.as_int();
            int height_mm = out.physical_size_mm.height.as_int();
            float inches =
                sqrtf(width_mm * width_mm + height_mm * height_mm) / 25.4;

            logger->log(component, severity,
                        "%sPhysical size %.1f\" %dx%dmm",
                        indent, inches, width_mm, height_mm);

            static const char* const power_mode[] =
                {"on", "in standby", "suspended", "off"};
            logger->log(component, severity,
                        "%sPower is %s", indent, power_mode[out.power_mode]);

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

                static const char* const orientation[] =
                    {"normal", "left", "inverted", "right"};
                int degrees_ccw = out.orientation;
                logger->log(component, severity,
                            "%sOrientation %s",
                            indent,
                            orientation[degrees_ccw / 90]);

                logger->log(component, severity,
                            "%sLogical size %dx%d",
                            indent,
                            out.extents().size.width.as_int(),
                            out.extents().size.height.as_int());

                logger->log(component, severity,
                            "%sLogical position %+d%+d",
                            indent,
                            out.top_left.x.as_int(),
                            out.top_left.y.as_int());

                logger->log(component, severity,
                            "%sScaling factor: %.2f",
                            indent,
                            out.scale);
            }
        }
    });
}

void mrl::DisplayConfigurationReport::catastrophic_configuration_error(
    std::shared_ptr<mg::DisplayConfiguration const> const& failed_fallback,
    std::exception const& error)
{
    logger->log(component, ml::Severity::critical, "Failed to revert to safe display configuration!");
    logger->log(component, ml::Severity::critical, "Attempted to fall back to configuration:");
    log_configuration(ml::Severity::critical, *failed_fallback);
    logger->log(component, ml::Severity::critical, "Error details:");
    logger->log(component, ml::Severity::critical, "%s", boost::diagnostic_information(error).c_str());
}

void mrl::DisplayConfigurationReport::configuration_updated_for_session(
    std::shared_ptr<scene::Session> const& session,
    std::shared_ptr<graphics::DisplayConfiguration const> const& config)
{
    logger->log(component, severity, "Sending display configuration to session %s:", session->name().c_str());
    log_configuration(severity, *config);
}
