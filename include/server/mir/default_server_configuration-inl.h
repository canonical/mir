/*
 * Copyright © 2012 Canonical Ltd.
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
 * Authored by: Andreas Pokorny <andreas.pokorny@canoncial.com>
 */

#ifndef MIR_DEFAULT_SERVER_CONFIGURATION_INL_H_
#define MIR_DEFAULT_SERVER_CONFIGURATION_INL_H_

#include "mir/default_server_configuration.h"
#include "mir/abnormal_exit.h"

template <typename Report>
auto mir::DefaultServerConfiguration::create_report(std::shared_ptr<Report>(mir::ReportFactory::*factory_function)(), char const* report_opt) const
    -> std::shared_ptr<Report>
{
    auto opt = the_options()->get<std::string>(report_opt);

    if (opt == log_opt_value)
    {
        return (logging_report_factory.get()->*factory_function)();
    }
    else if (opt == lttng_opt_value)
    {
        return (lttng_report_factory.get()->*factory_function)();
    }
    else if (opt == off_opt_value)
    {
        return (null_report_factory.get()->*factory_function)();
    }
    else
    {
        throw AbnormalExit(std::string("Invalid ") + report_opt + " option: " + opt + " (valid options are: \"" +
                           ConfigurationOptions::off_opt_value + "\" and \"" + ConfigurationOptions::log_opt_value +
                           "\" and \"" + ConfigurationOptions::lttng_opt_value + "\")");
    }
}

#endif

