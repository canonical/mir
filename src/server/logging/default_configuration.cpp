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
 * Authored by: Alan Griffiths <alan@octopull.co.uk>
 */

#include "mir/default_server_configuration.h"
#include "mir/abnormal_exit.h"

#include "surfaces_report.h"

namespace ml = mir::logging;
namespace ms = mir::surfaces;

std::shared_ptr<ms::SurfacesReport>
mir::DefaultServerConfiguration::the_surfaces_report()
{
    return surfaces_report([this]() -> std::shared_ptr<ms::SurfacesReport>
    {
        auto opt = the_options()->get(surfaces_report_opt, off_opt_value);

        if (opt == log_opt_value)
        {
            return std::make_shared<ml::SurfacesReport>(the_logger());
        }
        else if (opt == off_opt_value)
        {
            return std::make_shared<ms::NullSurfacesReport>();
        }
        else
        {
            throw AbnormalExit(std::string("Invalid ") + surfaces_report_opt + " option: " + opt +
                " (valid options are: \"" + off_opt_value + "\" and \"" + log_opt_value + "\")");
        }
    });
}
