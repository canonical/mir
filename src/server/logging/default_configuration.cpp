/*
 * Copyright © 2013-2014 Canonical Ltd.
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

#include "mir/logging/logger.h"
#include "mir/logging/glog_logger.h"
#include "mir/default_server_configuration.h"
#include "mir/logging/dumb_console_logger.h"

namespace ml = mir::logging;

auto mir::DefaultServerConfiguration::the_logger()
    -> std::shared_ptr<ml::Logger>
{
    return logger(
        [this]() -> std::shared_ptr<ml::Logger>
        {
            if (the_options()->is_set(glog))
            {
                return std::make_shared<ml::GlogLogger>(
                    "mir",
                    the_options()->get<int>(glog_stderrthreshold),
                    the_options()->get<int>(glog_minloglevel),
                    the_options()->get<std::string>(glog_log_dir));
            }
            else
            {
                return std::make_shared<ml::DumbConsoleLogger>();
            }
        });
}


